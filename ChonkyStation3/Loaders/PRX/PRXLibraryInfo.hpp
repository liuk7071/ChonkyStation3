#pragma once

#include <common.hpp>


struct PRXSegmentInfo {
    u64 addr;
    u64 type;
    u64 file_size;
    u64 mem_size;
};

struct PRXLibraryInfo {
    std::string name = "";
    std::string filename = "";
    u32 id = 0;
    u32 toc = 0;
    u32 prologue_func = 0;
    u32 epilogue_func = 0;
    u32 start_func = 0;
    u32 stop_func = 0;
    
    std::vector<PRXSegmentInfo> segs;
};
