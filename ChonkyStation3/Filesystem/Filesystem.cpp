#include "Filesystem.hpp"
#include "PlayStation3.hpp"


void Filesystem::mount(Filesystem::Device device, fs::path path) {
    mounted_devices[device] = path;
    log("Mounted device %s at %s\n", deviceToString(device).c_str(), path.generic_string().c_str());
}

void Filesystem::umount(Filesystem::Device device) {
    if (!mounted_devices.contains(device)) Helpers::panic("Tried to unmount an unmounted device (%s)\n", deviceToString(device).c_str());

    mounted_devices.erase(device);
    log("Unmounted device %s\n", deviceToString(device).c_str());
}

void Filesystem::initialize() {
    // Create mount point directories if they don't exist
    for (auto& i : mounted_devices)
        fs::create_directories(i.second);

    // Initialize directory structures
    if (isDeviceMounted(Device::DEV_HDD0)) {
        fs::create_directories(guestPathToHost("/dev_hdd0/game"));
        fs::create_directories(guestPathToHost("/dev_hdd0/home/00000001"));
    }

    if (isDeviceMounted(Device::DEV_FLASH)) {
        fs::create_directories(guestPathToHost("/dev_flash/sys/external"));
        fs::create_directories(guestPathToHost("/dev_flash/data/cert"));
    }
}

u32 Filesystem::open(fs::path path, u32 flags) {
    const fs::path host_path = ps3->fs.guestPathToHost(path);
    const bool create = flags & CELL_FS_O_CREAT;
    
    std::string mode = "rb";
    if (flags & CELL_FS_O_ACCMODE) {    // unnecessary I think
        if (flags & CELL_FS_O_RDONLY)      mode = "rb";
        else if (flags & CELL_FS_O_WRONLY) mode = "wb";
        else mode = "rb+";
    }
    
    if (flags & CELL_FS_O_EXCL) {
        Helpers::panic("TODO: CELL_FS_O_EXCL\n");
    }
    
    if (flags & CELL_FS_O_APPEND) {
        Helpers::panic("TODO: CELL_FS_O_APPEND\n");
    }
    
    if (flags & CELL_FS_O_MSELF) {
        Helpers::panic("TODO: CELL_FS_O_MSELF");
    }
    
    if (!fs::exists(host_path)) {
        if (!create) {
            //Helpers::panic("Tried to open non-existing file %s\n", path.generic_string().c_str());
            log("WARNING: Tried to open non-existing file %s\n", path.generic_string().c_str());
            return 0;
        } else {
            // Create the file if it didn't exist and the create flag was specified
            std::ofstream temp = std::ofstream(host_path);
            temp.close();
        }
    }

    const u32 new_file_id = ps3->handle_manager.request();
    FILE* file = std::fopen(host_path.generic_string().c_str(), mode.c_str());
    if (!file) {
        Helpers::panic("Failed to open file %s with mode %s\n", host_path.generic_string().c_str(), mode.c_str());
    }
    open_files[new_file_id] = { file, host_path, path, flags };
    log("Opened file %s\n", host_path.generic_string().c_str());
    return new_file_id;
}

u32 Filesystem::opendir(fs::path path) {
    const fs::path host_path = ps3->fs.guestPathToHost(path);
    if (!fs::exists(host_path)) {
        log("WARNING: Tried to open non-existing dir %s\n", path.generic_string().c_str());
        return 0;
    }

    const u32 new_file_id = ps3->handle_manager.request();
    open_dirs[new_file_id] = { path, 0 };
    log("Opened directory %s\n", path.generic_string().c_str());
    return new_file_id;
}

void Filesystem::close(u32 file_id) {
    FILE* file = getFileFromID(file_id).file;
    std::fclose(file);
    open_files.erase(file_id);
}

void Filesystem::closedir(u32 file_id) {
    // TODO
}

u64 Filesystem::read(u32 file_id, u32 buf_ptr, u64 size) {
    // Read up to PAGE_SIZE at max, split into multiple reads if larger
    FILE* file = getFileFromID(file_id).file;
    u64 bytes_read = 0;
    u32 cur_ptr = buf_ptr;
    u64 size_remaining = size;
    
    // Make the first read read up to page boundary, so that the next reads are page aligned
    const u64 to_read = std::min(size_remaining, ps3->mem.ram.pageAlign(buf_ptr) - buf_ptr);
    bytes_read += std::fread(ps3->mem.getPtr(cur_ptr), sizeof(u8), to_read, file);
    size_remaining -= to_read;
    cur_ptr += to_read;
    
    while (size_remaining > 0) {
        const u64 to_read = std::min(PAGE_SIZE, size_remaining);
        bytes_read += std::fread(ps3->mem.getPtr(cur_ptr), sizeof(u8), to_read, file);
        size_remaining -= to_read;
        cur_ptr += to_read;
    }
    
    return bytes_read;
}

u64 Filesystem::seek(u32 file_id, s64 offs, u32 mode) {
    FILE* file = getFileFromID(file_id).file;
#ifdef _WIN32
    _fseeki64(file, offs, mode);
#else
    std::fseek(file, offs, mode);
#endif
    return std::ftell(file);
}

u64 Filesystem::tell(u32 file_id) {
    FILE* file = getFileFromID(file_id).file;
    return std::ftell(file);
}

// Returns false if path already exists
bool Filesystem::mkdir(fs::path path) {
    const fs::path host_path = ps3->fs.guestPathToHost(path);
    if (fs::exists(host_path)) return false;

    fs::create_directories(host_path);
    return true;
}

u64 Filesystem::getFileSize(u32 file_id) {
    auto file = getFileFromID(file_id);
    
    // If the file doesn't exist but the flag to create it was specified, return 0 size
    if ((file.flags & CELL_FS_O_CREAT) && !fs::exists(file.path)) {
        return 0;
    }
    
    return fs::file_size(file.path);
}

u64 Filesystem::getFileSize(fs::path path) {
    return fs::file_size(guestPathToHost(path));
}

bool Filesystem::isDirectory(u32 file_id) {
    auto file = getFileFromID(file_id);
    return fs::is_directory(file.path);
}

bool Filesystem::isDirectory(fs::path path) {
    return fs::is_directory(guestPathToHost(path));
}

bool Filesystem::exists(fs::path path) {
    return fs::exists(guestPathToHost(path));
}

Filesystem::File& Filesystem::getFileFromID(u32 id) {
    if (!open_files.contains(id))
        Helpers::panic("File id %d does not exist\n", id);
    return open_files[id];
}

Filesystem::Directory& Filesystem::getDirFromID(u32 id) {
    if (!open_dirs.contains(id))
        Helpers::panic("Dir id %d does not exist\n", id);
    return open_dirs[id];
}

bool Filesystem::isDeviceMounted(Filesystem::Device device) {
    return mounted_devices.contains(device);
}

bool Filesystem::isDeviceMounted(fs::path path) {
    return mounted_devices.contains(getDeviceFromPath(path));
}

fs::path Filesystem::guestPathToHost(fs::path path) {
    const std::string path_str = path.generic_string();
    fs::path guest_path;

    // Check if the path is valid
    // TODO: I assume relative paths can exist too...?
    if (path.begin()->generic_string() != "/")
        Helpers::panic("Path %s is not valid\n", path.generic_string().c_str());

    // Check if device is valid
    std::string device_str = std::next(path.begin(), 1)->generic_string();
    Device device = stringToDevice(device_str);
    if (device == Device::INVALID)
        Helpers::panic("Path %s: device %s is not a valid device\n", path_str.c_str(), device_str.c_str());

    // Check if device is mounted
    if (!isDeviceMounted(device))
        Helpers::panic("Path %s: device %s is not mounted\n", path_str.c_str(), device_str.c_str());

    // Check if this path contains only the device
    int count = 0;
    for (const auto& i : path) count++;
    if (count == 2 || (count == 3 && path.filename().empty())) {
        return mounted_devices[device];
    }
    
    // Convert to host path
    std::string path_no_device_start = std::next(path.begin(), 2)->generic_string();
    fs::path path_no_device = path_str.substr(path_str.find(path_no_device_start));
    
    guest_path = mounted_devices[device] / path_no_device;
    return guest_path;
}

Filesystem::Device Filesystem::getDeviceFromPath(fs::path path) {
    std::string device_str = std::next(path.begin(), 1)->generic_string();
    return stringToDevice(device_str);
}

bool Filesystem::isValidDevice(fs::path path) {
    const auto device = getDeviceFromPath(path);
    return device != Device::INVALID;
}

std::string Filesystem::deviceToString(Filesystem::Device device) {
    if (device == Device::INVALID) Helpers::panic("deviceToString: invalid device\n");

    switch (device) {
    case Device::DEV_FLASH:     return "dev_flash";
    case Device::DEV_HDD0:      return "dev_hdd0";
    case Device::DEV_HDD1:      return "dev_hdd1";
    case Device::DEV_USB000:    return "dev_usb000";
    case Device::DEV_USB001:    return "dev_usb001";
    case Device::DEV_USB002:    return "dev_usb002";
    case Device::DEV_USB003:    return "dev_usb003";
    case Device::DEV_USB004:    return "dev_usb004";
    case Device::DEV_USB005:    return "dev_usb005";
    case Device::DEV_USB006:    return "dev_usb006";
    case Device::DEV_USB007:    return "dev_usb007";
    case Device::DEV_MS:        return "dev_ms";
    case Device::DEV_CF:        return "dev_cf";
    case Device::DEV_SD:        return "dev_sd";
    case Device::DEV_BDVD:      return "dev_bdvd";
    case Device::APP_HOME:      return "app_home";
    }
}

Filesystem::Device Filesystem::stringToDevice(std::string device) {
    if (device == "dev_flash")          return Device::DEV_FLASH;
    else if (device == "dev_hdd0")      return Device::DEV_HDD0;
    else if (device == "dev_hdd1")      return Device::DEV_HDD1;
    else if (device == "dev_usb000")    return Device::DEV_USB000;
    else if (device == "dev_usb001")    return Device::DEV_USB001;
    else if (device == "dev_usb002")    return Device::DEV_USB002;
    else if (device == "dev_usb003")    return Device::DEV_USB003;
    else if (device == "dev_usb004")    return Device::DEV_USB004;
    else if (device == "dev_usb005")    return Device::DEV_USB005;
    else if (device == "dev_usb006")    return Device::DEV_USB006;
    else if (device == "dev_usb007")    return Device::DEV_USB007;
    else if (device == "dev_ms")        return Device::DEV_MS;
    else if (device == "dev_cf")        return Device::DEV_CF;
    else if (device == "dev_sd")        return Device::DEV_SD;
    else if (device == "dev_bdvd")      return Device::DEV_BDVD;
    else if (device == "app_home")      return Device::APP_HOME;
    else return Device::INVALID;
}
