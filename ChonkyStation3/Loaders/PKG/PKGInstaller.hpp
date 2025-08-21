#pragma once

#include <common.hpp>
#include <logger.hpp>
#include <BEField.hpp>

#include <functional>

#include <aes.hpp>
#include <plusaes/plusaes.hpp>


// Each file will be split in buffers of this size, and only 1 buffer will be loaded in memory at once
static constexpr size_t BUFFER_SIZE = 1_MB;

// Circular dependency
class PlayStation3;

class PKGInstaller {
public:
    PKGInstaller(PlayStation3* ps3) : ps3(ps3) {}
    PlayStation3* ps3;
    
    // Reads package info, but doesn't install it
    void load(const fs::path& path);
    // Get a specific file in the package, and write it in guest_out
    bool getFile(const fs::path& path, const fs::path& guest_out);
    // Same as getFile but async
    void getFileAsync(const fs::path& path, const fs::path& guest_out, std::function<void(bool)> on_complete);
    // Install the package loaded with load()
    bool install(std::function<void(float)> signal_progress = nullptr);
    // Same as install but async
    void installAsync(std::function<void(bool)> on_complete, std::function<void(float)> signal_progress = nullptr);
    // Cancel installation
    void cancel();
    
    struct PKGHeader {
        BEField<u32>    magic;
        BEField<u16>    pkg_revision;
        BEField<u16>    pkg_type;
        BEField<u32>    pkg_metadata_offset;
        BEField<u32>    pkg_metadata_count;
        BEField<u32>    pkg_metadata_size;
        BEField<u32>    item_count;
        BEField<u64>    total_size;
        BEField<u64>    data_offset;
        BEField<u64>    data_size;
        u8              content_id[0x30];
        u8              digest[0x10];
        u8              pkg_data_riv[0x10];
        u8              pkg_header_digest[0x40];
    };
    
    struct PKGItemRecord {
       BEField<u32> filename_offset;
       BEField<u32> filename_size;
       BEField<u64> data_offset;
       BEField<u64> data_size;
       BEField<u32> flags;
       BEField<u32> padding;
     };
    
    fs::path pkg_path;
    
    // Metadata
    std::string title;
    std::string title_id;
    u64 size_in_bytes;
    char content_id[0x31];  // +1 to add a null terminator
    u32 drm_type = 0;
    
private:
    PKGHeader header;
    PKGItemRecord* file_list;
    const u8 npdrm_pkg_ps3_key[16] =    { 0x2E, 0x7B, 0x71, 0xD7, 0xC9, 0xC9, 0xA1, 0x4E,
                                          0xA3, 0x22, 0x1F, 0x18, 0x88, 0x28, 0xB8, 0xF8 };
    
    
    void fixIV(u8* iv, u64 offset, u8* out_iv);
    void createFile(fs::path path, PKGItemRecord& record);
    
    MAKE_LOG_FUNCTION(log, loader_pkg);
};
