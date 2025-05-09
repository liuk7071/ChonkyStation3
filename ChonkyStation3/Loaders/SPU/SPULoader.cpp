#include <Loaders/ELF/ELFLoader.hpp>
#include "SPULoader.hpp"
#include "PlayStation3.hpp"


using namespace ELFIO;

void SPULoader::load(u32 img_ptr, sys_spu_image* img) {
    elfio elf;
    u8* img_src = ps3->mem.getPtr(img_ptr);

    // Try to dump the ELF to a file and load it
    // We don't know the actual size of the ELF yet, so we use the maximum possible size (~513KB)
    // (each segment is at most 16KB, and there can be max 32 segments, 16KB * 32 = 512KB, add 1KB for the ELF headers even if they're a few bytes)
    std::string filename = std::format("{:08x}.elf", img_ptr);
    std::ofstream file(filename, std::ios::binary);
    file.write((char*)img_src, 513_KB);
    file.close();

    load(filename, img, true);
}

// I am aware that a lot of this code is copy pasted from the ELF loader
void SPULoader::load(fs::path path, sys_spu_image* img, bool remove) {
    elfio elf;
    const std::string filename = path.filename().generic_string();

    if (!elf.load(path.generic_string())) {
        Helpers::panic("Couldn't load SPU image %s:\n%s\n", filename.c_str(), elf.validate().c_str());
    }

    // Allocate segment table (there can be max 32 segments)
    const u32 segs_ptr = ps3->mem.alloc(sizeof(sys_spu_segment) * 32)->vaddr;
    sys_spu_segment* segs = (sys_spu_segment*)ps3->mem.getPtr(segs_ptr);
    int n_segs = 0;

    log("Loading SPU image %s\n", filename.c_str());
    log("* %d segments\n", elf.segments.size());
    for (int i = 0; i < elf.segments.size(); i++) {
        auto seg = elf.segments[i];
        // Skip the segment if it's empty
        if (seg->get_memory_size() == 0) {
            log("* Segment %d type %s: empty\n", i, ELFLoader::segment_type_string[seg->get_type()].c_str());
            continue;
        }
        log("* Segment %d type %s: 0x%016llx -> 0x%016llx\n", i, ELFLoader::segment_type_string[seg->get_type()].c_str(), seg->get_virtual_address(), seg->get_virtual_address() + seg->get_memory_size());

        // Allocate PT_LOAD segments 
        if (seg->get_type() == PT_LOAD) {
            const auto size = seg->get_memory_size();

            // Does this segment have any data?
            if (seg->get_file_size()) {
                auto addr = ps3->mem.alloc(seg->get_memory_size())->vaddr;
                std::memcpy(ps3->mem.getPtr(addr), seg->get_data(), seg->get_file_size());
                std::memset(ps3->mem.getPtr(addr + seg->get_file_size()), 0, size - seg->get_file_size());  // Redundant, FILL type segments are for this

                segs[n_segs].type = SYS_SPU_SEGMENT_TYPE_COPY;
                segs[n_segs].ls_addr = seg->get_virtual_address();
                segs[n_segs].src.addr = addr;
                segs[n_segs].size = seg->get_memory_size();

                n_segs++;
            }

            if (size > seg->get_file_size()) {
                segs[n_segs].type = SYS_SPU_SEGMENT_TYPE_FILL;
                segs[n_segs].ls_addr = seg->get_virtual_address() + seg->get_file_size();
                segs[n_segs].src.addr = 0;
                segs[n_segs].size = seg->get_memory_size() - seg->get_file_size();

                n_segs++;
            }

            if (size < seg->get_file_size()) {
                Helpers::panic("SPULoader::load: segment file size > memory size\n");
            }
        }
        else if (seg->get_type() == INFO) {
            segs[n_segs].type = SYS_SPU_SEGMENT_TYPE_INFO;
            // TODO
            n_segs++;
        }
        else {
            Helpers::panic("SPULoader::load: unimplemented segment type %d\n", seg->get_type());
        }
    }

    const u32 entry = elf.get_entry();
    log("Entry: 0x%08x\n", entry);
    img->entry = entry;
    img->segs_ptr = segs_ptr;
    img->n_segs = n_segs;

    if (remove) fs::remove(filename);
}