#pragma once

#include <common.hpp>
#include <logger.hpp>
#include <BEField.hpp>
#include <elfio/elfio.hpp>

#include <format>
#include <unordered_map>

#include <PRX/PRXExport.hpp>
#include <PRX/PRXLibraryInfo.hpp>
#include <StubPatcher.hpp>


static constexpr u32 SCE_PPURELA = 0x700000a4;

// Circular dependency
class PlayStation3;

class PRXLoader {
public:
    PRXLoader(PlayStation3* ps3) : ps3(ps3) {}
    PlayStation3* ps3;
    
    struct PRXRelocation {
        BEField<u64> offs;
        BEField<u16> unk;
        u8 data_idx;    // Segment index for relocated data
        u8 addr_idx;    // Segment index for relocated address (to write the data to)
        BEField<u32> type;
        BEField<u64> ptr;
    };

    struct PRXLibrary {
        BEField<u16> attrs;
        u8 ver[2];
        u8 name[28];
        BEField<u32> toc;
        BEField<u32> exports_start;
        BEField<u32> exports_end;
        BEField<u32> imports_start;
        BEField<u32> imports_end;
    };

    struct PRXModule {
        u8 size;
        u8 unk0;
        BEField<u16> ver;
        BEField<u16> attrs;
        BEField<u16> n_funcs;
        BEField<u16> n_vars;
        BEField<u16> n_tls;
        u8 info_hash;
        u8 info_tlshash;
        u8 unk1[2];
        BEField<u32> name_ptr;
        BEField<u32> nids_ptr;
        BEField<u32> addrs_ptr;
        BEField<u32> var_nids_ptr;
        BEField<u32> var_stubs_ptr;
        BEField<u32> unk2;
        BEField<u32> unk3;
    };
    
    PRXLibraryInfo load(const fs::path& path, PRXExportTable& exports);
    void exportModules(const u32 exports_start, const u32 exports_end, PRXExportTable& exports, PRXLibrary* lib = nullptr, u32* prologue_func = nullptr, u32* epilogue_func = nullptr, u32* start_func = nullptr, u32* stop_func = nullptr);
    std::string getSpecialFunctionName(const u32 nid);

    std::unordered_map<u64, std::string> segment_type_string {
        { ELFIO::PT_LOAD,   "PT_LOAD    " },
        { ELFIO::PT_TLS,    "PT_TLS     " },
        { SCE_PPURELA,      "SCE_PPURELA" },
    };

    std::unordered_map<u32, std::string> special_function_names {
        { 0x0d10fd3f, "module_prologue" },
        { 0x330f7005, "module_epilogue" },
        { 0xab779874, "module_stop" },
        { 0xbc9a0086, "module_start" },
    };

private:
    MAKE_LOG_FUNCTION(log, loader_prx);
};
