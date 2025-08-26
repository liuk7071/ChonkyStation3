#include "ELFLoader.hpp"
#include "PlayStation3.hpp"


using namespace ELFIO;

u64 ELFLoader::load(const fs::path& path, std::unordered_map<u32, u32>& imports, PROCParam& proc_param, ModuleManager& module_manager) {
    elfio elf;

    auto str = path.generic_string();
    if (!elf.load(str.c_str())) {
        Helpers::panic("Couldn't load ELF %s:\n%s\n", str.c_str(), elf.validate().c_str());
    }

    log("Loading ELF %s\n", str.c_str());
    log("* %d segments\n", elf.segments.size());
    for (int i = 0; i < elf.segments.size(); i++) {
        auto seg = elf.segments[i];
        // Skip the segment if it's empty
        if (seg->get_memory_size() == 0) {
            log("* Segment %d type %s: empty\n", i, segment_type_string[seg->get_type()].c_str());
            continue;
        }
        log("* Segment %d type %s: 0x%016llx -> 0x%016llx\n", i, segment_type_string[seg->get_type()].c_str(), seg->get_virtual_address(), seg->get_virtual_address() + seg->get_memory_size());
        
        // PT_TLS
        if (seg->get_type() == PT_TLS) {
            tls_vaddr = seg->get_virtual_address();
            tls_filesize = seg->get_file_size();
            tls_memsize = seg->get_memory_size();
        }

        // PROC_PARAM
        else if (seg->get_type() == PROC_PARAM) {
            PROCParam* proc_param_ptr = (PROCParam*)seg->get_data();
            proc_param = *proc_param_ptr;
            log("version            : 0x%08x\n", (u32)proc_param.version);
            log("sdk_version        : 0x%08x\n", (u32)proc_param.sdk_version);
            log("primary_stacksize  : 0x%08x\n", (u32)proc_param.primary_stacksize);
            log("malloc_pagesize    : 0x%08x\n", (u32)proc_param.malloc_pagesize);
        }
        
        // PRX_PARAM
        else if (seg->get_type() == PRX_PARAM) {
            // Check prx magic
            PRXParam* prx_param = (PRXParam*)seg->get_data();
            if (prx_param->magic != PRX_MAGIC) {
                Helpers::panic("PRX PARAM wrong magic (0x%08x)", prx_param->magic);
            }

            log("size         : 0x%08x\n", (u32)prx_param->size);
            log("magic        : 0x%08x\n", (u32)prx_param->magic);
            log("version      : 0x%08x\n", (u32)prx_param->version);
            log("libentstart  : 0x%08x\n", (u32)prx_param->libentstart);
            log("libstubstart : 0x%08x\n", (u32)prx_param->libstubstart);
            log("libstubend   : 0x%08x\n", (u32)prx_param->libstubend);
            log("ver          : 0x%08x\n", (u16)prx_param->ver);

            // Patch stubs
            for (int i = prx_param->libstubstart; i < prx_param->libstubend; i += sizeof(PRXStubHeader)) {
                PRXStubHeader* stub = (PRXStubHeader*)mem.getPtr(i);

                /*log("Found stub\n");
                log("s_modulename : 0x%016x\n", (u32)stub->s_modulename);
                log("s_imports    : %d\n", (u32)stub->s_imports);*/

                if (stub->s_modulename == 0) {
                    log("Skipping null module\n");
                    continue;
                }

                std::string name;
                u8* namePtr = mem.getPtr(stub->s_modulename);
                name = Helpers::readString(namePtr);
                const bool lle = ps3->prx_manager.isLLEModule(name);        // Is this a LLE module?
                const bool user = !CellTypes::module_list.contains(name);   // Is this an user module? (Exported by a custom SPRX)
                if (lle)
                    ps3->prx_manager.require(name);

                log("Found module %s %s with %d imports\n", name.c_str(), lle ? "(LLE)" : (user ? "(user)" : ""), (u16)stub->s_imports);

                for (int i = 0; i < stub->s_imports; i++) {
                    u32 nid = mem.read<u32>(stub->s_nid + i * 4);
                    u32 addr = mem.read<u32>(stub->s_text + i * 4);
                    imports[addr] = nid;
                    log("* Imported function: 0x%08x @ 0x%08x \t[%s]\n", nid, addr, module_manager.getImportName(nid).c_str());

                    StubPatcher::patch(addr, ps3->module_manager.isForcedHLE(nid) ? false : (lle || user), ps3);
                }
                log("\n");
            }
            log("\n");
        }

        // Load segment only if it's of type PT_LOAD
        if (seg->get_type() == PT_LOAD) {
            const u64 size = seg->get_memory_size();
            const u64 aligned_size = mem.ram.pageAlign(size);
            const u64 paddr = mem.ram.allocPhys(size)->start;
            auto entry = mem.ram.mmap(seg->get_virtual_address(), paddr, size);
            for (u64 addr = entry->vaddr; addr < entry->vaddr + entry->size; addr += PAGE_SIZE)
                mem.markAsFastMem(addr >> PAGE_SHIFT, mem.getPtr(addr), true, true);

            std::memcpy(mem.getPtr(seg->get_virtual_address()), seg->get_data(), seg->get_file_size());
            // Set the remaining memory to 0
            std::memset(mem.getPtr(seg->get_virtual_address()) + seg->get_file_size(), 0, seg->get_memory_size() - seg->get_file_size());
        }
    }

    // The entry point in the ELF header is actually an address from which you read the actual entry point
    u64 entry = mem.read<u32>(elf.get_entry());
    log("* Entry point: 0x%016llx (0x%016llx)\n", elf.get_entry(), entry);
    log("\n");
    // But we return the address from the ELF header (need it outside this class, r2 is initialized to [entry + 4] on boot)
    return elf.get_entry();
}
