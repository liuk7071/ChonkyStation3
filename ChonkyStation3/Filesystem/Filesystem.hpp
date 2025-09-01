#pragma once

#include <common.hpp>
#include <logger.hpp>

#include <unordered_map>


// Circular dependency
class PlayStation3;

class Filesystem {
public:
    Filesystem(PlayStation3* ps3) : ps3(ps3) {}
    PlayStation3* ps3;

    enum class Device {
        DEV_FLASH,
        DEV_HDD0,
        DEV_HDD1,
        DEV_USB000,
        DEV_USB001,
        DEV_USB002,
        DEV_USB003,
        DEV_USB004,
        DEV_USB005,
        DEV_USB006,
        DEV_USB007,
        DEV_MS,
        DEV_CF,
        DEV_SD,
        DEV_BDVD,
        APP_HOME,
        INVALID
    };

    struct File {
        FILE* file;
        fs::path path;
        fs::path guest_path;
        u32 flags = 0;
    };

    struct Directory {
        fs::path path;
        int cur = 0;
    };

    std::unordered_map<Device, fs::path> mounted_devices;
    std::unordered_map<u32, File> open_files;
    std::unordered_map<u32, Directory> open_dirs;
    
    void mount(Device device, fs::path path);
    void umount(Device device);
    void initialize();
    u32 open(fs::path path, u32 flags = 0);
    u32 opendir(fs::path path);
    void close(u32 file_id);
    void closedir(u32 file_id);
    u64 read(u32 file_id, u32 buf_ptr, u64 size);
    u64 write(u32 file_id, u32 buf_ptr, u64 size);
    u64 seek(u32 file_id, s64 offs, u32 mode);
    u64 tell(u32 file_id);
    bool mkdir(fs::path path);
    u64 getFileSize(u32 file_id);
    u64 getFileSize(fs::path path);
    bool isDirectory(u32 file_id);
    bool isDirectory(fs::path path);
    bool exists(fs::path path);
    File& getFileFromID(u32 id);
    Directory& getDirFromID(u32 id);
    bool isDeviceMounted(Device device);
    bool isDeviceMounted(fs::path path);
    fs::path guestPathToHost(fs::path path);
    Device getDeviceFromPath(fs::path path);
    bool isValidDevice(fs::path path);
    static std::string deviceToString(Device device);
    static Device stringToDevice(std::string device);

private:
    MAKE_LOG_FUNCTION(log, filesystem);
};
