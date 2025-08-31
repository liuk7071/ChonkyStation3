#include <Syscall.hpp>
#include "PlayStation3.hpp"


MAKE_LOG_FUNCTION(log_sys_fs, sys_fs);

u64 Syscall::sys_fs_test() {
    const u32 arg0 = ARG0;
    const u32 arg1 = ARG1;
    const u32 arg2 = ARG2;
    const u32 arg3 = ARG3;
    const u32 buf_ptr = ARG4;
    const u32 buf_size = ARG5;
    log_sys_fs("sys_fs_test(arg0: %d, arg1: %d, arg2: 0x%08x, arg3: %d, buf_ptr: 0x%08x, buf_size: %d)\n", arg0, arg1, arg2, arg3, buf_ptr, buf_size);

    const u32 fd = ps3->mem.read<u32>(arg2);
    auto file = ps3->fs.getFileFromID(fd);

    std::memset((char*)ps3->mem.getPtr(buf_ptr), 0, buf_size);
    std::strncpy((char*)ps3->mem.getPtr(buf_ptr), file.guest_path.generic_string().c_str(), buf_size);
    
    return CELL_OK;
}

u64 Syscall::sys_fs_fcntl() {
    const u32 fd = ARG0;
    const u32 op = ARG1;
    const u32 arg_ptr = ARG2;
    const u32 size = ARG3;
    log_sys_fs("sys_fs_fcntl(fd: %d, op: 0x%08x, arg_ptr: 0x%08x, size: %d) UNIMPLEMENTED\n", fd, op, arg_ptr, size);

    switch (op) {
    case 0xc0000002: {  // Used by cellFsGetFreeSize
        ps3->mem.write<u32>(arg_ptr + 0x1c, 4096);
        ps3->mem.write<u64>(arg_ptr + 0x20, 20_GB / 4096);
        break;
    }
    }
    
    return CELL_OK;
}

u64 Syscall::sys_fs_fget_block_size() {
    const u32 fd = ARG0;
    const u32 sector_size_ptr = ARG1;   // sector_size is u64
    const u32 block_size_ptr = ARG2;    // block_size is u64
    const u32 arg4 = ARG3;  // ???
    const u32 out_flags_ptr = ARG4;
    log_sys_fs("sys_fs_fget_block_size(fd: %d, sector_size_ptr: 0x%08x, block_size_ptr: 0x%08x, arg4: 0x%08x, out_flags_ptr: 0x%08x)\n", fd, sector_size_ptr, block_size_ptr, arg4, out_flags_ptr);
    
    // TODO: Different devices have different sector and block sizes
    ps3->mem.write<u64>(sector_size_ptr, 512);
    ps3->mem.write<u64>(block_size_ptr, 4096);
    ps3->mem.write<u64>(out_flags_ptr, 0);  // TODO: Are these the flags passed to sys_fs_open?
    
    return CELL_OK;
}
