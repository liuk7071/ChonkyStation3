#include "PRXLoader.hpp"
#include "PlayStation3.hpp"


using namespace ELFIO;

PRXLibraryInfo PRXLoader::load(const fs::path& path, PRXExportTable& exports) {
    const auto elf_binary = Helpers::readBinary(path);
    elfio elf;

    // TODO: SPRX (encrypted)
    if (elf_binary[0] == 'S' && elf_binary[1] == 'C' && elf_binary[2] == 'E') {
        Helpers::panic("PRXLoader: PRX is encrypted (%s)\n", path.generic_string().c_str());
    }

    auto str = path.generic_string();
    if (!elf.load(str.c_str())) {
        Helpers::panic("Couldn't load PRX %s", path.generic_string().c_str());
    }

    log("\n");
    log("Loading PRX %s\n", path.filename().generic_string().c_str());
    log("* %d segments\n", elf.segments.size());
    // Allocate segments
    std::vector<u32> allocations;
    for (int i = 0; i < elf.segments.size(); i++) {
        auto seg = elf.segments[i];
        // Skip the segment if it's empty
        if (seg->get_memory_size() == 0) {
            log("* Segment %d type %s: empty\n", i, segment_type_string[seg->get_type()].c_str());
            continue;
        }
        log("* Segment %d type %s: 0x%016llx -> 0x%016llx\n", i, segment_type_string[seg->get_type()].c_str(), seg->get_virtual_address(), seg->get_virtual_address() + seg->get_memory_size());

        if (seg->get_type() == PT_LOAD) {
            // Allocate segment in memory
            auto entry = ps3->mem.alloc(seg->get_memory_size(), 0, true);
            allocations.push_back(entry->vaddr);
            u8* ptr = ps3->mem.ram.getPtrPhys(entry->paddr);
            ps3->mem.markAsFastMem(entry->vaddr >> PAGE_SHIFT, ptr, true, true);
            std::memcpy(ptr, seg->get_data(), seg->get_file_size());
            // Set the remaining memory to 0
            std::memset(ptr + seg->get_file_size(), 0, seg->get_memory_size() - seg->get_file_size());
        }
    }

    // Do relocations
    for (int i = 0; i < elf.segments.size(); i++) {
        auto seg = elf.segments[i];

        if (seg->get_type() == SCE_PPURELA) {
            for (int j = 0; j < seg->get_file_size(); j += sizeof(PRXRelocation)) {
                PRXRelocation* reloc = (PRXRelocation*)&seg->get_data()[j];
                //log("Relocation: offs: 0x%08x, addr_idx: %d, data_idx: %d, type: %d, ptr: 0x%016llx\n", (u64)reloc->offs, (u8)reloc->addr_idx, (u8)reloc->data_idx, (u32)reloc->type, (u64)reloc->ptr);
                Helpers::debugAssert(reloc->addr_idx < elf.segments.size(), "PRXLoader: addr segment idx is out of range\n");
                Helpers::debugAssert(reloc->data_idx < elf.segments.size(), "PRXLoader: data segment idx is out of range\n");

                const u32 base = reloc->data_idx != 0xff ? allocations[reloc->data_idx] : 0;
                const u64 data = base + reloc->ptr;
                const u32 reloc_addr = allocations[reloc->addr_idx] + reloc->offs;

                //log("Relocated address: ");
                switch (reloc->type) {
                case 1: {
                    ps3->mem.write<u32>(reloc_addr, data);
                    //logNoPrefix("[0x%08x] <- 0x%08x\n", reloc_addr, data);
                    break;
                }

                case 4: {
                    ps3->mem.write<u16>(reloc_addr, data);
                    //logNoPrefix("[0x%08x] <- 0x%04x\n", reloc_addr, data);
                    break;
                }

                case 5: {
                    ps3->mem.write<u16>(reloc_addr, data >> 16);
                    //logNoPrefix("[0x%08x] <- 0x%04x\n", reloc_addr, data >> 16);
                    break;
                }

                case 6: {
                    const u16 reloc_data = (u16)(data >> 16) + ((data >> 15) & 1);
                    ps3->mem.write<u16>(reloc_addr, reloc_data);
                    //logNoPrefix("[0x%08x] <- 0x%04x\n", reloc_addr, reloc_data);
                    break;
                }

                default:
                    Helpers::panic("PRXLoader: unimplemented relocation type %d\n", (u32)reloc->type);
                }
            }
        }
    }

    // Get exported functions
    PRXLibrary* lib = (PRXLibrary*)ps3->mem.getPtr(allocations[0] + elf.segments[0]->get_physical_address() - elf.segments[0]->get_offset());
    log("PRX exports library %s\n", lib->name);
    log("* attributes: 0x%08x\n", lib->attrs);
    log("* exports start: 0x%08x\n", (u32)lib->exports_start);
    log("* exports end  : 0x%08x\n", (u32)lib->exports_end);

    u32 start_func = 0;
    u32 stop_func = 0;

    int module_idx = 0;
    for (u32 addr = lib->exports_start; addr < lib->exports_end; module_idx++) {
        PRXModule* module = (PRXModule*)ps3->mem.getPtr(addr);
        log("%d:\n", module_idx);

        // Special nameless module for start and stop functions
        if (module->attrs & 0x8000) {
            for (int i = 0; i < module->n_funcs; i++) {
                const u32 nid  = ps3->mem.read<u32>(module->nids_ptr  + i * sizeof(u32));
                const u32 addr = ps3->mem.read<u32>(module->addrs_ptr + i * sizeof(u32));
                log("* Exported function: %s @ 0x%08x\n", getSpecialFunctionName(nid).c_str(), addr);
                
                switch (nid) {
                case 0xbc9a0086: start_func = addr; break;
                }
            }
            for (int i = 0; i < module->n_vars; i++) {
                const u32 nid = ps3->mem.read<u32>(module->nids_ptr + module->n_funcs + i * sizeof(u32));
                const u32 addr = ps3->mem.read<u32>(module->addrs_ptr + module->n_funcs + i * sizeof(u32));
                log("* Exported variable: 0x%08x @ 0x%08x\n", nid, addr);
                // TODO
            }
            if (module->size)
                addr += module->size;
            else
                addr += sizeof(PRXModule);
            continue;
        }

        log("Library %s exports module %s (%d functions, %d variables)\n", lib->name, ps3->mem.getPtr(module->name_ptr), (u16)module->n_funcs, (u16)module->n_vars);
        //Helpers::debugAssert(module->n_vars == 0, "PRX module: n_vars > 0\n");
        if (module->n_vars != 0) log("WARNING: exporting module with variables (unimplemented)\n");

        for (int i = 0; i < module->n_funcs; i++) {
            const u32 nid = ps3->mem.read<u32>(module->nids_ptr + i * sizeof(u32));
            const u32 addr = ps3->mem.read<u32>(module->addrs_ptr + i * sizeof(u32));
            exports.funcs[nid] = { addr, allocations[0] + lib->toc };
            log("* Exported function: 0x%08x @ 0x%08x \t[%s]\n", nid, addr, ps3->module_manager.getImportName(nid).c_str());
        }

        if (module->size)
            addr += module->size;
        else
            addr += sizeof(PRXModule);
    }

    // Imports
    for (u32 addr = lib->imports_start; addr < lib->imports_end; ) {
        PRXModule* module = (PRXModule*)ps3->mem.getPtr(addr);
        const std::string name = Helpers::readString(ps3->mem.getPtr(module->name_ptr));

        log("Library %s imports module %s (%d functions, %d variables)\n", lib->name, name.c_str(), (u16)module->n_funcs, (u16)module->n_vars);
        
        const bool lle = ps3->prx_manager.isLLEModule(name);  // Is this a LLE module?
        if (lle)
            ps3->prx_manager.require(name);

        for (int i = 0; i < module->n_funcs; i++) {
            const u32 nid = ps3->mem.read<u32>(module->nids_ptr + i * sizeof(u32));
            const u32 addr = ps3->mem.read<u32>(module->addrs_ptr + i * sizeof(u32));
            ps3->module_manager.registerImport(addr, nid);
            log("* Imported function: 0x%08x @ 0x%08x \t[%s]\n", nid, addr, ps3->module_manager.getImportName(nid).c_str());
            StubPatcher::patch(addr, ps3->module_manager.isForcedHLE(nid) ? false : lle, ps3);
        }

        if (module->size)
            addr += module->size;
        else
            addr += sizeof(PRXModule);
    }
    log("\n");

    if (path.filename() == "liblv2.prx") {
        start_func = 0;
    }

    return { Helpers::readString(lib->name), path.filename().generic_string(), lib->toc, start_func, stop_func };
}

std::string PRXLoader::getSpecialFunctionName(const u32 nid) {
    if (special_function_names.contains(nid))
        return special_function_names[nid];
    else
        return std::format("unk_special_{:08x}", nid);
}
