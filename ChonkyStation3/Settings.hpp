#pragma once

#include <common.hpp>

#include <SDL.h>
#include <toml.hpp>


class Settings {
public:
    void load();
    void save();

    bool detected_broken_config = false;

    // --- The following functions must be called after load()---

    // Default settings

    struct {
        std::string nickname = "Alber";
    } system;

    struct {
        bool partialLv2LLE = true;
        bool sys_fs = false;
        bool cellResc = true;
        bool cellPngDec = false;
        bool cellJpgDec = false;
        bool cellFont = false;
        bool cellFontFT = false;
        bool cell_FreeType2 = false;
        bool cellSync = true;
        bool cellSync2 = true;
        bool cellSpurs = true;
        bool cellSpursJq = true;
        bool cellKey2char = true;
        bool cellL10n = true;
        bool cellFiber = true;
        bool cellSail = false;

        inline bool isLLEEnabled(std::string name) {
            if (name == "sys_fs")               return sys_fs;
            else if (name == "cellResc")        return cellResc;
            else if (name == "cellPngDec")      return cellPngDec;
            else if (name == "cellJpgDec")      return cellJpgDec;
            else if (name == "cellFont")        return cellFont;
            else if (name == "cellFontFT")      return cellFontFT;
            else if (name == "cell_FreeType2")  return cell_FreeType2;
            else if (name == "cellSync")        return cellSync;
            else if (name == "cellSync2")       return cellSync2;
            else if (name == "cellSpurs")       return cellSpurs;
            else if (name == "cellSpursJq")     return cellSpursJq;
            else if (name == "cellKey2char")    return cellKey2char;
            else if (name == "cellL10n")        return cellL10n;
            else if (name == "cellFiber")       return cellFiber;
            else if (name == "cellSail")        return cellSail;
            //else Helpers::panic("Tried to get LLE configuration of unknown module %s\n", name.c_str());
            else return true;
        }
    } lle;

    struct {
        std::string dev_hdd0_mountpoint     = "./Filesystem/dev_hdd0";
        std::string dev_hdd1_mountpoint     = "./Filesystem/dev_hdd1";
        std::string dev_flash_mountpoint    = "./Filesystem/dev_flash";
        std::string dev_usb000_mountpoint   = "./Filesystem/dev_usb000";
    } filesystem;
    
    struct {
        std::string backend = "Null";
    } audio;

    struct {
        bool pause_on_start = false;
        bool disable_spu = false;
        std::string enable_spu_after_pc = "";
        std::string spu_thread_to_enable = "";
        bool dont_step_cellaudio_port_read_idx = true;
    } debug;
};
