#include "PKGInstaller.hpp"
#include "PlayStation3.hpp"


#ifdef _WIN32
#define seek _fseeki64
#else
#define seek std::fseek
#endif

static AES_ctx ctx;

void PKGInstaller::load(const fs::path& path) {
    const auto path_str = path.generic_string();
    
    log("Loading PKG %s\n", path_str.c_str());
    file = std::fopen(path_str.c_str(), "rb");
    if (!file) {
        Helpers::panic("Failed to open file %s\n", path_str.c_str());
    }
    
    // Read PKG header
    seek(file, 0, SEEK_SET);
    std::fread(&header, sizeof(PKGHeader), 1, file);
    // Verify magic
    if (std::strncmp((char*)&header.magic, "\x7fPKG", 4)) {
        Helpers::panic("PKG Loader: invalid pkg magic\n");
    }
    
    // Read the content ID
    std::memset(content_id, 0, 0x31);
    std::memcpy(content_id, header.content_id, 0x30);
    
    log("Content ID: %s\n", content_id);
    log("PKG contains %d files\n", (u32)header.item_count);
    
    // Read metadata
    log("PKG has %d metadata packets\n", (u32)header.pkg_metadata_count);
    u8* metadata = new u8[header.pkg_metadata_size];
    seek(file, header.pkg_metadata_offset, SEEK_SET);
    std::fread(metadata, header.pkg_metadata_size, 1, file);
    u32 curr_offs = 0;
    for (int i = 0; i < header.pkg_metadata_count; i++) {
        const u32 packet_id     = Helpers::bswap<u32>(*(u32*)&metadata[curr_offs]);
        const u32 packet_size   = Helpers::bswap<u32>(*(u32*)&metadata[curr_offs + sizeof(u32)]);
        curr_offs += sizeof(u32) * 2;
        log("Metadata: id 0x%02x, size %d\n", packet_id, packet_size);
        
        switch (packet_id) {
        default: {
            curr_offs += packet_size;
            // Align up to 4 bytes
            curr_offs = (curr_offs + 3) & ~3;
            break;
        }
        }
    }
    delete[] metadata;
    
    // Decrypt file list
    file_list = new PKGItemRecord[header.item_count];
    seek(file, header.data_offset, SEEK_SET);
    std::fread(file_list, sizeof(PKGItemRecord) * header.item_count, 1, file);

    log("AES IV: ");
    for (int i = 0; i < 16; i++)
        logNoPrefix("%02x", header.pkg_data_riv[i]);
    logNoPrefix("\n");
    AES_init_ctx_iv(&ctx, npdrm_pkg_ps3_key, header.pkg_data_riv);
    AES_CTR_xcrypt_buffer(&ctx, (u8*)file_list, sizeof(PKGItemRecord) * header.item_count);
    
    // Get PARAM.SFO
    for (int i = 0; i < header.item_count; i++) {
        //log("File entry:\n");
        //log("Filename offset: 0x%llx\n", (u64)file_list[i].filename_offset);
        //log("Filename size  : %d\n",     (u32)file_list[i].filename_size);
        //log("Data offset    : 0x%llx\n", (u64)file_list[i].data_offset);
        //log("Data size      : %lld\n",   (u32)file_list[i].data_size);
        //log("Flags          : 0x%08x\n", (u32)file_list[i].data_size);
        
        // Get filename
        char* filename = new char[file_list[i].filename_size + 1];  // Add a null terminator to be safe
        std::memset(filename, 0, file_list[i].filename_size + 1);
        seek(file, header.data_offset + file_list[i].filename_offset, SEEK_SET);
        std::fread(filename, file_list[i].filename_size, 1, file);
        
        u8 iv[16];
        fixIV(header.pkg_data_riv, file_list[i].filename_offset, iv);
        AES_ctx_set_iv(&ctx, iv);
        AES_CTR_xcrypt_buffer(&ctx, (u8*)filename, file_list[i].filename_size);
        //log("%s\n", filename);
        
        const auto file_path = fs::path(filename);
        log("%s\n", file_path.generic_string().c_str());
        if (file_path.filename() == "PARAM.SFO") {
            createFile("/dev_hdd1/", file_list[i]);
            
            // Load SFO
            SFOLoader sfo (ps3->fs);
            auto data = sfo.parse("/dev_hdd1/PARAM.SFO");
            title       = (char*)data.strings["TITLE"].data();
            title_id    = (char*)data.strings["TITLE_ID"].data();
            log("Title    : %s\n", title.c_str());
            log("Title ID : %s\n", title_id.c_str());
            
            fs::remove(ps3->fs.guestPathToHost("/dev_hdd1/PARAM.SFO"));
            delete[] filename;
            break;
        }
        
        delete[] filename;
    }
}

void PKGInstaller::install() {
    Helpers::debugAssert(file, "PKGInstaller: called install() before load()");
    
    // TODO: Multithread this
    
    fs::path install_dir = fs::path("/dev_hdd0/game") / title_id;
    log("Installing %s to %s...\n", content_id, install_dir.generic_string().c_str());
    
    // Create files
    for (int i = 0; i < header.item_count; i++) {
        // Get filename
        char* filename = new char[file_list[i].filename_size + 1];  // Add a null terminator to be safe
        std::memset(filename, 0, file_list[i].filename_size + 1);
        seek(file, header.data_offset + file_list[i].filename_offset, SEEK_SET);
        std::fread(filename, file_list[i].filename_size, 1, file);
        
        u8 iv[16];
        fixIV(header.pkg_data_riv, file_list[i].filename_offset, iv);
        AES_ctx_set_iv(&ctx, iv);
        AES_CTR_xcrypt_buffer(&ctx, (u8*)filename, file_list[i].filename_size);
        
        createFile(install_dir, file_list[i]);
        log(" [%d/%d] Created file %s\n", i + 1, (u32)header.item_count, filename);
        delete[] filename;
    }
    
    log("Installed successfully\n");
}

void PKGInstaller::fixIV(u8* iv, u64 offset, u8* out_iv) {
    std::memcpy(out_iv, iv, 16);
    
    // Add block_offset to iv
    u64 block_offset = offset / 16;
    for (int i = 15; i >= 0 && block_offset > 0; i--) {
        u64 sum = iv[i] + (block_offset & 0xff);
        out_iv[i] = (u8)(sum & 0xff);
        block_offset = (block_offset >> 8) + (sum >> 8);
    }
}

void PKGInstaller::createFile(fs::path path, PKGItemRecord& record) {
    // Get filename
    char* filename = new char[record.filename_size + 1];  // Add a null terminator to be safe
    std::memset(filename, 0, record.filename_size + 1);
    seek(file, header.data_offset + record.filename_offset, SEEK_SET);
    std::fread(filename, record.filename_size, 1, file);

    u8 iv[16];
    fixIV(header.pkg_data_riv, record.filename_offset, iv);
    AES_ctx_set_iv(&ctx, iv);
    AES_CTR_xcrypt_buffer(&ctx, (u8*)filename, record.filename_size);
    
    const auto guest_path = path / std::string(filename);
    const auto host_path = ps3->fs.guestPathToHost(guest_path);
    delete[] filename;
    
    // Check if it's a directory
    if (record.flags & 4) {
        fs::create_directories(host_path);
        return;
    }
    
    fs::create_directories(host_path.parent_path());
    FILE* new_file = std::fopen(host_path.generic_string().c_str(), "wb");
    if (!new_file) {
        Helpers::panic("Failed to create file %s\n", host_path.generic_string().c_str());
    }
    
    auto remaining_size = record.data_size;
    for (int i = 0; i < record.data_size; i += BUFFER_SIZE) {
        const auto buf_size = std::min<u64>(BUFFER_SIZE, remaining_size);
        const u64 data_offs = record.data_offset + i;
        
        // Read the buffer
        u8* buf = new u8[buf_size];
        seek(file, header.data_offset + data_offs, SEEK_SET);
        std::fread(buf, buf_size, 1, file);
        
        // Decrypt it
        u8 iv[16];
        fixIV(header.pkg_data_riv, data_offs, iv);
        AES_ctx_set_iv(&ctx, iv);
        AES_CTR_xcrypt_buffer(&ctx, (u8*)buf, buf_size);
        
        // Write it
        std::fwrite(buf, buf_size, 1, new_file);
        delete[] buf;
    }
    
    std::fclose(new_file);
}
