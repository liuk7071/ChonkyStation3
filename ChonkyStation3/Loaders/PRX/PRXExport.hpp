#pragma once

#include <common.hpp>


struct PRXExport {
    u32 addr;
    //u32 toc;
};

struct PRXExportTable {
    std::unordered_map<u32, PRXExport> funcs;
};
