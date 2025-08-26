#pragma once

#include <common.hpp>
#include <CellTypes.hpp>
#include <logger.hpp>
#include <BEField.hpp>
#include <elfio/elfio.hpp>

#include <unordered_map>

#include <Memory.hpp>
#include <ModuleManager.hpp>
#include <StubPatcher.hpp>


//constexpr u32 PT_LOAD = 0x00000001; // Already defined in ELFIO
//constexpr u32 PT_TLS = 0x00000007; // Already defined in ELFIO
// PS3 specific
static constexpr u32 PROC_PARAM = 0x60000001;
static constexpr u32 PRX_PARAM  = 0x60000002;

static constexpr u32 PROC_MAGIC = 0x13bcc5f6;
static constexpr u32 PRX_MAGIC  = 0x1b434cec;

// Circular dependency
class PlayStation3;

class ELFLoader {
public:
    ELFLoader(PlayStation3* ps3, Memory& mem) : ps3(ps3), mem(mem) {}
    PlayStation3* ps3;
    Memory& mem;

    static inline std::unordered_map<u64, std::string> segment_type_string {
        { ELFIO::PT_LOAD,   "PT_LOAD    " },
        { ELFIO::PT_TLS,    "PT_TLS     " },
        { PROC_PARAM,       "PROC_PARAM " },
        { PRX_PARAM,        "PRX_PARAM  " },
    };

    struct PROCParam {
        BEField<u32> size;
        BEField<u32> magic;
        BEField<u32> version;
        BEField<u32> sdk_version;
        BEField<u32> primary_prio;
        BEField<u32> primary_stacksize;
        BEField<u32> malloc_pagesize;
        BEField<u32> ppc_seg;
    };

    struct PRXParam {
        BEField<u32> size;
        BEField<u32> magic;
        BEField<u32> version;
        BEField<u32> pad0;
        BEField<u32> libentstart;
        BEField<u32> libentend;
        BEField<u32> libstubstart;
        BEField<u32> libstubend;
        BEField<u16> ver;
        BEField<u16> pad1;
        BEField<u32> pad2;
    };

    struct PRXStubHeader {
        u8 s_size;
        u8 s_unk0;
        BEField<u16> s_version;
        BEField<u16> s_unk1;
        BEField<u16> s_imports;
        BEField<u32> s_unk2;
        BEField<u32> s_unk3;
        BEField<u32> s_modulename;
        BEField<u32> s_nid;
        BEField<u32> s_text;
        BEField<u32> s_unk4;
        BEField<u32> s_unk5;
        BEField<u32> s_unk6;
        BEField<u32> s_unk7;
    };

    u64 load(const fs::path& path, std::unordered_map<u32, u32>& imports, PROCParam& proc_param, ModuleManager& module_manager);

    u32 tls_vaddr = 0;
    u32 tls_filesize = 0;
    u32 tls_memsize = 0;

private:
    MAKE_LOG_FUNCTION(log, loader_elf);
};
