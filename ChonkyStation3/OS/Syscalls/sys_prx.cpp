#include <Syscall.hpp>
#include "PlayStation3.hpp"


MAKE_LOG_FUNCTION(log_sys_prx, sys_prx);


struct sys_prx_module_info_t {
    BEField<u64> size;
    u8 name[30];
    u8 version[2];
    BEField<u32> modattribute;
    BEField<u32> start_entry;
    BEField<u32> stop_entry;
    BEField<u32> all_segments_num;
    BEField<u32> filename_ptr;
    BEField<u32> filename_size;
    BEField<u32> segments_ptr;
    BEField<u32> n_segments;
};

struct sys_prx_get_module_list_t {
    BEField<u64> size;
    BEField<u32> pad0;
    BEField<u32> max;
    BEField<u32> count;
    BEField<u32> id_ptr;
    BEField<u32> unk0;
};

struct sys_prx_module_info_option_t {
    BEField<u64> size;
    sys_prx_module_info_t info;
};

u64 Syscall::sys_prx_get_module_list() {
    const u64 flags = ARG0;
    const u32 info_ptr = ARG1;
    log_sys_prx("sys_prx_get_module_list(flags: 0x%016llx, info_ptr: 0x%08x)\n", flags, info_ptr);

    sys_prx_get_module_list_t* info = (sys_prx_get_module_list_t*)ps3->mem.getPtr(info_ptr);
    BEField<u32>* ids = (BEField<u32>*)ps3->mem.getPtr(info->id_ptr);
    int i;
    for (i = 0; i < info->max; i++) {
        if (i < ps3->prx_manager.prxs.size()) {
            ids[i] = ps3->prx_manager.prxs[i].id;
        }
        else {
            break;
        }
    }
    info->count = i;

    return CELL_OK;
}

u64 Syscall::sys_prx_get_module_info() {
    const u32 id = ARG0;
    const u64 flags = ARG1;
    const u32 opt_ptr = ARG2;
    log_sys_prx("sys_prx_get_module_info(id: %d, flags: 0x%016llx, info_ptr: 0x%08x)\n", id, flags, opt_ptr);

    sys_prx_module_info_option_t* opt = (sys_prx_module_info_option_t*)ps3->mem.getPtr(opt_ptr);
    PRXManager::Lv2PRX* prx = ps3->prx_manager.getLv2PRXById(id);
    if (prx) {
        std::strncpy((char*)opt->info.name, (char*)prx->path.filename().generic_string().c_str(), 30);  // This is wrong, should be module name, not filename
    }
    else {
        Helpers::panic("Could not get PRX with id %d\n", id);
    }

    return CELL_OK;
}

u64 Syscall::sys_prx_get_module_id_by_name() {
    const u32 name_ptr = ARG0;
    const u64 flags = ARG1;
    const u32 opt_ptr = ARG2;
    const std::string name = Helpers::readString(ps3->mem.getPtr(name_ptr));
    log_sys_prx("sys_prx_get_module_id_by_name(name_ptr: 0x%08x, flags: 0x%016llx, opt_ptr: 0x%08x) [name: %s]\n", name_ptr, flags, opt_ptr, name.c_str());

    // libprof is a profiling library unavailable on retail consoles
    if (name == "cellLibprof") return 0x8001112e;   // CELL_PRX_ERROR_UNKNOWN_MODULE

    return ps3->handle_manager.request();
}