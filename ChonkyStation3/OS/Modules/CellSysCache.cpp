#include "CellSysCache.hpp"
#include "PlayStation3.hpp"


u64 CellSysCache::cellSysCacheMount() {
    const u32 param_ptr = ARG0;
    log("cellSysCacheMount(param_ptr: 0x%08x)\n", param_ptr);

    CellSysCacheParam* param = (CellSysCacheParam*)ps3->mem.getPtr(param_ptr);
    cache_path = "/dev_hdd1/caches/" + ps3->curr_game.id + "/" + Helpers::readString(param->cache_id);
    Helpers::debugAssert(cache_path.generic_string().size() <= CELL_SYSCACHE_PATH_MAX, "cellSysCacheMount: path %s is too long\n", cache_path.generic_string().c_str());

    log("Mounting cache at %s\n", cache_path.generic_string().c_str());
    fs::create_directories(ps3->fs.guestPathToHost(cache_path));
    std::strcpy((char*)param->cache_path, cache_path.generic_string().c_str());
    

    return CELL_OK;
}

u64 CellSysCache::cellSysCacheClear() {
    log("cellSysCacheClear()\n");
    Helpers::debugAssert(!cache_path.empty(), "cellSysCacheClear: cache was not mounted\n");

    // Clear the cache
    for (const auto& entry : fs::directory_iterator(ps3->fs.guestPathToHost(cache_path))) {
        fs::remove_all(entry.path());
    }

    return CELL_OK; // CELL_SYSCACHE_RET_OK_CLEARED
}
