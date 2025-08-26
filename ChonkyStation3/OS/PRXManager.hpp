#pragma once

#include <common.hpp>
#include <logger.hpp>

#include <unordered_map>

#include <PRX/PRXLibraryInfo.hpp>


// Circular dependency
class PlayStation3;

class PRXManager {
public:
    PRXManager(PlayStation3* ps3);
    PlayStation3* ps3;

    // This is a list of modules that *need* to be LLEd.
    // If the modules aren't present and the game requires them, the emulator will panic.
    // The map maps the module name to the library filename.
    // TODO: Switch these to the .sprx variants once we can decrypt SELFs/SPRXs.
    // For now you need to decrypt the libraries on your own.
    std::unordered_map<std::string, std::string> lle_modules;

    fs::path lib_dir = "/dev_flash/sys/external/";

    std::vector<std::string> required_modules;  // List of required modules, load them with loadModules()
    std::vector<PRXLibraryInfo> libs;   // List of loaded libraries

    bool isLLEModule(const std::string name);
    bool isLibLoaded(const std::string name);
    PRXLibraryInfo getLib(u32 id);
    void require(const std::string name);
    bool loadModule(const fs::path& path, u32* id = nullptr);  // Return true if the module was loaded
    bool loadModules(); // Returns true if at least 1 module was loaded
    void loadModulesRecursively();
    void initializeLibraries();

    // Lv2 PRX objects used by sys_prx
    struct Lv2PRX {
        u32 id;
        fs::path path;
    };
    std::vector<Lv2PRX> prxs;
    void createLv2PRXs();
    Lv2PRX* getLv2PRXById(u32 id);

private:
    MAKE_LOG_FUNCTION(log, manager_prx);
};
