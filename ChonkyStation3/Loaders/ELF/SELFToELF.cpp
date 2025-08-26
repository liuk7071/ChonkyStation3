#include "SELFToELF.hpp"
#include "PlayStation3.hpp"


#ifdef _WIN32
#define seek _fseeki64
#else
#define seek std::fseek
#endif

static void strToBytes(std::string str, u8* out) {
    u8* cur = out;
    for (int i = 0; i < str.size(); i += 2) {
        auto [ptr, ec] = std::from_chars(str.data() + i, str.data() + i + 2, *cur++, 16);
        Helpers::debugAssert(ec == std::errc(), "strToBytes error");
    }
}

int SELFToELF::makeELF(const fs::path& path, const fs::path& out_path) {
    auto path_str = path.generic_string();
    log("Loading SELF %s\n", path_str.c_str());
    FILE* file = std::fopen(path_str.c_str(), "rb");
    Helpers::debugAssert(file, "SELFToELF: failed to open file\n");
    
    // Get the Certified File Header
    CFHeader header;
    seek(file, 0, SEEK_SET);
    std::fread(&header, sizeof(CFHeader), 1, file);
    if (std::strncmp((char*)&header.magic, "SCE\0", 4))
        Helpers::panic("SELFToELF: file %s is not a valid certified file\n", path_str.c_str());
    
    if (!header.ext_header_size) {
        Helpers::panic("SELFToELF: certified file %s is not a SELF or SPRX\n", path_str.c_str());
    }
    
    ExtendedHeader ext_header;
    seek(file, sizeof(CFHeader), SEEK_SET);
    std::fread(&ext_header, sizeof(ExtendedHeader), 1, file);
    
    if (!ext_header.supplemental_hdr_offset) {
        Helpers::panic("SELFToELF: SELF %s does not have NPDRM (todo)\n", path_str.c_str());
    }
    
    // Find NPDRM packet
    SupplementalHeader sup_header;
    u64 cur_offs = ext_header.supplemental_hdr_offset;
    bool found_npd_packet = false;
    do {
        seek(file, cur_offs, SEEK_SET);
        std::fread(&sup_header, sizeof(SupplementalHeader), 1, file);
        log("Supplemental header: type %d\n", (u64)sup_header.type);
        // Check if supplemental header is of type NPDRM (3)
        if (sup_header.type == 3) {
            found_npd_packet = true;
            break;
        }
        cur_offs += sup_header.size;
    } while (sup_header.next);
    
    if (!found_npd_packet) {
        Helpers::panic("SELFToELF: SELF %s does not have NPDRM (todo)\n", path_str.c_str());
    }
    
    // Get the Program Identification Header
    ProgramIdentificationHeader prog_id_header;
    seek(file, ext_header.program_identification_header_offset, SEEK_SET);
    std::fread(&prog_id_header, sizeof(ProgramIdentificationHeader), 1, file);
    
    const u32 type      = prog_id_header.prog_type;
    const u16 revision  = header.attr;
    const u64 version   = prog_id_header.prog_sceversion;
    log("Type: %d, revision: %d, version: %d\n", type, revision, version);
    
    if (revision == 0x8000 || revision == 0xc000) {
        log("Debug SELF detected\n");
        
        Helpers::debugAssert(revision == 0x8000, "SELFToELF: Debug SELF revision 0xc000 (todo)");
        
        // Get file offset and size directly and write to file
        const u64 elf_offs = revision == 0x8000 ? header.file_offset : Helpers::bswap<u64>(header.file_offset);
        const u64 elf_size = revision == 0x8000 ? header.file_size : Helpers::bswap<u64>(header.file_size);  // I added the assert above so I will remember to verify if this is swapped too
        log("Debug SELF offset : 0x%x\n", elf_offs);
        log("Debug SELF size   : %d\n", elf_size);
        
        u8* buf = new u8[elf_size];
        seek(file, elf_offs, SEEK_SET);
        std::fread(buf, elf_size, 1, file);
        
        FILE* out = std::fopen(out_path.generic_string().c_str(), "wb");
        if (!out) {
            Helpers::panic("SELFToELF: Could not open %s for writing\n", out_path.generic_string().c_str());
        }
        std::fwrite(buf, elf_size, 1, out);
        
        log("Done\n");
        std::fclose(file);
        std::fclose(out);
        delete[] buf;
        return;
    }
    
    u8 content_id[0x31];    // Add a null terminator
    std::memset(content_id, 0, 0x31);
    std::memcpy(content_id, sup_header.npd.content_id, 0x30);
    log("Found NPDRM packet\n");
    log("Content ID: %s\n", content_id);
    
    // Get RAP file path
    const std::string rap_name = std::string((char*)content_id) + ".rap";
    const fs::path rap_path = ps3->getCurrentUserHomeDir() / "exdata" / rap_name;
    if (!ps3->fs.exists(rap_path)) {
        log("Could not find license file for content %s\n", content_id);
        return -1;
    }
    log("Found license: %s\n", rap_path.generic_string().c_str());
    
    // Get NPDRM Key License from the RAP file
    fs::path host_rap_path = ps3->fs.guestPathToHost(rap_path);
    FILE* rap_file = std::fopen(host_rap_path.generic_string().c_str(), "rb");
    if (!rap_file) {
        Helpers::panic("SELFToELF: Could not open license file %s\n", host_rap_path.generic_string().c_str());
    }
    u8 rap_data[16];
    seek(rap_file, 0, SEEK_SET);
    std::fread(rap_data, 16, 1, rap_file);
    log("RAP               : ");
    for (int i = 0; i < 16; i++)
        logNoPrefix("%02x", rap_data[i]);
    logNoPrefix("\n");
    
    // First round of decryption - AES128CBC
    u8 rap[16];
    u8 iv[16];
    std::memset(iv, 0, 16);
    if (int e = plusaes::decrypt_cbc(rap_data, 16, RAP_KEY, 16, &iv, rap, 16, nullptr)) {
        Helpers::panic("AES error (%d)\n", e);
    }

    // Next 5 rounds of whatever this is. Ported from RPCS3
    for (int round = 0; round < 5; round++) {
        for (int i = 0; i < 16; i++) {
            const int p = RAP_PBOX[i];
            rap[p] ^= RAP_E1[p];
        }
        for (int i = 15; i >= 1; i--) {
            const int p  = RAP_PBOX[i];
            const int pp = RAP_PBOX[i - 1];
            rap[p] ^= rap[pp];
        }
        int o = 0;
        for (int i = 0; i < 16; i++) {
            const int p             = RAP_PBOX[i];
            const unsigned char kc  = rap[p] - o;
            const unsigned char ec2 = RAP_E2[p];
            if (o != 1 || kc != 0xff) {
                o      = kc < ec2 ? 1 : 0;
                rap[p] = kc - ec2;
            }
            else if (kc == 0xff) {
                rap[p] = kc - ec2;
            }
            else {
                rap[p] = kc;
            }
        }
    }
    
    // Final round of decryption - AES128ECB.
    // This results in our NPDRM license key (KLIC).
    u8 klic[16];
    std::memset(iv, 0, 16);
    if (int e = plusaes::decrypt_ecb(rap, 16, NP_KLIC_KEY, 16, klic, 16, nullptr)) {
        Helpers::panic("AES error (%d)\n", e);
    }

    log("NPDRM License key : ");
    for (int i = 0; i < 16; i++)
        logNoPrefix("%02x", klic[i]);
    logNoPrefix("\n");
    
    // Get the Certified File Encryption Root Header, located after the Extended header
    // and encrypted first with AES256CBC using system keys
    // and then with AES128CBC using NPDRM KLIC.
    // The system keys vary depending on type, revision and version of the SELF
    
    // Get the SELF key
    SELFKey& self_key = getSELFKey(type, revision, version);
    u8 erk[32];
    u8 riv[16];
    self_key.getERK(erk);
    self_key.getRIV(riv);
    log("Got SELF Key\n");
    log("ERK: ");
    for (int i = 0; i < 32; i++)
        logNoPrefix("%02x", erk[i]);
    logNoPrefix("\n");
    log("RIV: ");
    for (int i = 0; i < 16; i++)
        logNoPrefix("%02x", riv[i]);
    logNoPrefix("\n");
    
    // Read Encryption Root Header data
    constexpr auto enc_root_hdr_size = sizeof(CFEncryptionRootHeader);
    u8 encryption_root_header_data[enc_root_hdr_size];
    seek(file, sizeof(CFHeader) + header.ext_header_size, SEEK_SET);
    std::fread(encryption_root_header_data, enc_root_hdr_size, 1, file);
    
    // First round of decryption - NPDRM layer
    std::memset(iv, 0, 16);
    u8 encryption_root_header_no_npdrm[enc_root_hdr_size];
    if (int e = plusaes::decrypt_cbc(encryption_root_header_data, enc_root_hdr_size, klic, 16, &iv, encryption_root_header_no_npdrm, enc_root_hdr_size, nullptr)) {
        Helpers::panic("AES error (%d)\n", e);
    }

    // Second round of decryption - AES256CBC with the system SELF key
    CFEncryptionRootHeader encryption_root_header;
    plusaes::decrypt_cbc(encryption_root_header_no_npdrm, enc_root_hdr_size, erk, 32, &riv, (u8*)&encryption_root_header, enc_root_hdr_size, nullptr);
    
    log("Decrypted Encryption Root Header\n");
    log("Key : ");
    for (int i = 0; i < 16; i++)
        logNoPrefix("%02x", encryption_root_header.key[i]);
    logNoPrefix("\n");
    log("RIV : ");
    for (int i = 0; i < 16; i++)
        logNoPrefix("%02x", encryption_root_header.iv[i]);
    logNoPrefix("\n");
    
    // Get the Certification Header and Segments, encrypted with the key from the Encryption Root Header via AES128CTR
    const auto cert_body_size = header.file_offset - (sizeof(CFHeader) + header.ext_header_size + sizeof(CFEncryptionRootHeader));
    u8 cert_data[cert_body_size];
    
    seek(file, sizeof(CFHeader) + header.ext_header_size + sizeof(CFEncryptionRootHeader), SEEK_SET);
    std::fread(cert_data, cert_body_size, 1, file);
    std::memcpy(iv, encryption_root_header.iv, 16);
    plusaes::crypt_ctr(cert_data, cert_body_size, encryption_root_header.key, 16, &iv);
    
    CertificationHeader certification_header;
    std::memcpy((u8*)&certification_header, cert_data, sizeof(CertificationHeader));
    log("Decrypted Certification Header\n");
    log("Number of certification segments   : %d\n", (u32)certification_header.cert_entry_num);
    log("Number of certification attributes : %d\n", (u32)certification_header.attr_entry_num);
    
    const u32 n_certs = certification_header.cert_entry_num;
    SegmentCertificationHeader segment_headers[n_certs];
    std::memcpy((u8*)segment_headers, cert_data + sizeof(CertificationHeader), sizeof(SegmentCertificationHeader) * n_certs);
    
    // Copy keys
    const u32 n_keys = certification_header.attr_entry_num * 16;
    u8 keys[n_keys];
    std::memcpy(keys, cert_data + sizeof(CertificationHeader) + sizeof(SegmentCertificationHeader) * n_certs, n_keys);
    
    // Load and write ELF header to file
    Elf64_Ehdr ehdr;
    seek(file, ext_header.ehdr_offset, SEEK_SET);
    std::fread((u8*)&ehdr, sizeof(Elf64_Ehdr), 1, file);
    log("SELF has %d program headers\n", (u16)ehdr.e_phnum);
    FILE* out = std::fopen(out_path.generic_string().c_str(), "wb");
    std::fwrite((u8*)&ehdr, sizeof(Elf64_Ehdr), 1, out);
    
    // Load and write program headers to file
    const int n_phdr = ehdr.e_phnum;
    Elf64_Phdr phdrs[n_phdr];
    seek(file, ext_header.phdr_offset, SEEK_SET);
    std::fread((u8*)&phdrs, sizeof(Elf64_Phdr) * n_phdr, 1, file);
    seek(out, ehdr.e_shoff, SEEK_SET);
    std::fwrite((u8*)&phdrs, sizeof(Elf64_Phdr) * n_phdr, 1, out);
    for (int i = 0; i < n_phdr; i++) {
        log("Program header %d: vaddr: 0x%08x, size: %d\n", i, (u32)phdrs[i].p_vaddr, (u32)phdrs[i].p_memsz);
    }
    
    // Load and write section headers to file
    const int n_shdr = ehdr.e_shnum;
    Elf64_Shdr shdrs[n_shdr];
    if (n_shdr) {
        seek(file, ext_header.shdr_offset, SEEK_SET);
        std::fread((u8*)&shdrs, sizeof(Elf64_Shdr) * n_shdr, 1, file);
        std::fwrite((u8*)&shdrs, sizeof(Elf64_Shdr) * n_shdr, 1, out);
    }

    // Decrypt Certification Segment data
    for (int i = 0; i < n_certs; i++) {
        auto& seg = segment_headers[i];
        log("Loading Certification Segment %d (encrypted: %s compressed: %s)\n", i, seg.enc_algorithm != 1 ? "yes," : "no, ", seg.comp_algorithm != 1 ? "yes" : "no");
        
        log("Type          : %d\n", (u32)seg.segment_type);
        log("Size          : %d\n", (u64)seg.segment_size);
        log("Offset        : 0x%x\n", (u64)seg.segment_offset);
        log("Segment index : %d\n", (u32)seg.segment_id);
        if (seg.enc_algorithm != 1) {
            log("Key index: %d, IV index: %d\n", (u32)seg.key_idx, (u32)seg.iv_idx);
            
            if (seg.comp_algorithm != 1) {
                // TODO: Compressed segments
                Helpers::panic("SELFToELF: compressed segment (todo)\n");
            }
            
            // Decrypt segment
            Helpers::debugAssert(seg.enc_algorithm == 3, "SELFToELF: encryption algorithm is not 3");
            Helpers::debugAssert(seg.key_idx < certification_header.attr_entry_num - 1, "SELFToELF: segment key idx is out of bounds");
            Helpers::debugAssert(seg.iv_idx < certification_header.attr_entry_num - 1, "SELFToELF: segment iv idx is out of bounds");
            Helpers::debugAssert(seg.segment_id < n_phdr, "SELFToELF: segment id is out of bounds\n");
            
            // Load encrypted data
            u8* data = new u8[seg.segment_size];
            seek(file, seg.segment_offset, SEEK_SET);
            std::fread(data, seg.segment_size, 1, file);
            
            // Decrypt it via AES128CTR
            u8 key[16];
            std::memcpy(key, keys + seg.key_idx * 16, 16);
            std::memcpy(iv,  keys + seg.iv_idx  * 16, 16);
            plusaes::crypt_ctr(data, seg.segment_size, key, 16, &iv);
            
            // Write decrypted data to file
            seek(out, phdrs[seg.segment_id].p_offset, SEEK_SET);
            std::fwrite(data, seg.segment_size, 1, out);
            delete[] data;
        }
    }
    
    log("Done\n");
    std::fclose(file);
    std::fclose(out);
    return 0;
}

void SELFToELF::SELFKey::getERK(u8* out) {
    strToBytes(erk, out);
}

void SELFToELF::SELFKey::getRIV(u8* out) {
    strToBytes(riv, out);
}

void SELFToELF::SELFKey::getPUB(u8* out) {
    strToBytes(pub, out);
}

void SELFToELF::SELFKey::getPRIV(u8* out) {
    strToBytes(priv, out);
}

SELFToELF::SELFKey& SELFToELF::getSELFKey(u32 type, u16 revision, u64 version) {
    auto find_key_with_revision = [&](u16 revision, SELFKey* keys, size_t n_keys) -> SELFKey& {
        for (int i = 0; i < n_keys; i++) {
            if (keys[i].revision == revision)
                return keys[i];
        }
        Helpers::panic("Could not find SELF key\n");
    };
    
    switch (type) {
    case SELF_TYPE_NPDRM: return find_key_with_revision(revision, npdrm_keys, 16);
            
    default:
        Helpers::panic("Unhandled SELF type %d\n", type);
    }
}
