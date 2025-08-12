#include "ModuleManager.hpp"
#include "PlayStation3.hpp"


//#define LOG_LLE_FUNC_RESULT

void ModuleManager::call(u32 nid) {
    if (!import_map.contains(nid)) {
        Helpers::panic("Unimplemented function unk_0x%08x\n", nid);
        last_call = getImportName(nid);
        ps3->ppu->state.gprs[3] = stub();
        return;
    }

    const auto import = import_map[nid];
    last_call = import.name;
    ps3->ppu->state.gprs[3] = import.handler();
}

void ModuleManager::lle(u32 nid) {
    if (!exports.funcs.contains(nid))
        Helpers::panic("Could not find export for function %s\n", getImportName(nid).c_str());

#ifndef CHONKYSTATION3_USER_BUILD

#ifdef LOG_LLE_FUNC_RESULT
    ps3->mem.watchpoints_r[ps3->ppu->state.lr + 4] = std::bind(&ModuleManager::printReturnValue, this, std::placeholders::_1);
    ps3->mem.markAsSlowMem((ps3->ppu->state.lr + 4) >> PAGE_SHIFT, true, false);
    last_lle_nid = nid;
#endif

#endif

    log("%s @ 0x%08x pc 0x%08x\n", getImportName(nid).c_str(), (u32)ps3->ppu->state.lr, (u32)ps3->ppu->state.pc);
    ps3->ppu->runFunc(ps3->mem.read<u32>(exports.funcs[nid].addr), ps3->mem.read<u32>(exports.funcs[nid].addr + 4), false);
}

void ModuleManager::registerImport(u32 addr, u32 nid) {
    imports[addr] = nid;
}

void ModuleManager::registerExportTable(const PRXExportTable& exports) {
    this->exports = exports;
}

std::string ModuleManager::getImportName(const u32 nid) {
    if (import_map.find(nid) == import_map.end())
        return std::format("unk_{:08x}", nid);
    else
        return import_map[nid].name;
}

bool ModuleManager::isForcedHLE(const u32 nid) {
    if (!import_map.contains(nid)) return false;
    else return import_map[nid].force_hle;
}

void ModuleManager::printReturnValue(u64 addr) {
    log("%s returned with 0x%08x\n", getImportName(last_lle_nid).c_str(), ps3->ppu->state.gprs[3]);
}

u64 ModuleManager::stub() {
    unimpl("%s() UNIMPLEMENTED @ 0x%08x\n", last_call.c_str(), ps3->ppu->state.lr);
    return CELL_OK;
}

void ModuleManager::init() {
    import_map = {
        { 0xe6f2c1e7, { "sysProcessExit",                                   std::bind(&SysPrxForUser::sysProcessExit, &sysPrxForUser), true }},
        { 0x2c847572, { "sysProcessAtExitSpawn",                            std::bind(&SysPrxForUser::sysProcessAtExitSpawn, &sysPrxForUser), true }},
        { 0x2d36462b, { "_sys_strlen",                                      std::bind(&SysPrxForUser::sysStrlen, &sysPrxForUser) }},
        { 0x8461e528, { "sysGetSystemTime",                                 std::bind(&SysPrxForUser::sysGetSystemTime, &sysPrxForUser), true }},
        { 0x96328741, { "sysProcess_At_ExitSpawn",                          std::bind(&SysPrxForUser::sysProcess_At_ExitSpawn, &sysPrxForUser), true }},
        { 0x5267cb35, { "sysSpinlockUnlock",                                std::bind(&SysPrxForUser::sysSpinlockUnlock, &sysPrxForUser) }},
        { 0x8c2bb498, { "sysSpinlockInitialize",                            std::bind(&SysPrxForUser::sysSpinlockInitialize, &sysPrxForUser) }},
        { 0x99c88692, { "_sys_strcpy",                                      std::bind(&SysPrxForUser::sysStrcpy, &sysPrxForUser) }},
        { 0xa285139d, { "sysSpinlockLock",                                  std::bind(&SysPrxForUser::sysSpinlockLock, &sysPrxForUser) }},
        { 0x4f7172c9, { "sys_process_is_stack",                             std::bind(&SysPrxForUser::sysProcessIsStack, &sysPrxForUser) }},
        { 0x9f04f7af, { "_sys_printf",                                      std::bind(&SysPrxForUser::sysPrintf, &sysPrxForUser) }},
        { 0x052d29a6, { "_sys_strcat",                                      std::bind(&SysPrxForUser::sysStrcat, &sysPrxForUser) }},
        { 0x996f7cf8, { "_sys_strncat",                                     std::bind(&SysPrxForUser::sysStrncat, &sysPrxForUser) }},
        { 0xd3039d4d, { "_sys_strncpy",                                     std::bind(&SysPrxForUser::sysStrncpy, &sysPrxForUser) }},
        { 0x68b9b011, { "_sys_memset",                                      std::bind(&SysPrxForUser::sysMemset, &sysPrxForUser) }},
        { 0x6bf66ea7, { "_sys_memcpy",                                      std::bind(&SysPrxForUser::sysMemcpy, &sysPrxForUser) }},
        { 0xbdb18f83, { "_sys_malloc",                                      std::bind(&SysPrxForUser::sysMalloc, &sysPrxForUser), true }},
        { 0xf7f7fb20, { "_sys_free",                                        std::bind(&SysPrxForUser::sysFree, &sysPrxForUser), true }},
        { 0xfb5db080, { "_sys_memcmp",                                      std::bind(&SysPrxForUser::sysMemcmp, &sysPrxForUser) }},

        { 0x1573dc3f, { "sysLwMutexLock",                                   std::bind(&SysLwMutex::sysLwMutexLock, &sysLwMutex), true }},
        { 0x1bc200f4, { "sysLwMutexUnlock",                                 std::bind(&SysLwMutex::sysLwMutexUnlock, &sysLwMutex), true }},
        { 0x2f85c0ef, { "sysLwMutexCreate",                                 std::bind(&SysLwMutex::sysLwMutexCreate, &sysLwMutex), true }},
        { 0xaeb78725, { "sysLwMutexTryLock",                                std::bind(&SysLwMutex::sysLwMutexTryLock, &sysLwMutex), true }},
        { 0xc3476d0c, { "sysLwMutexDestroy",                                std::bind(&SysLwMutex::sysLwMutexDestroy, &sysLwMutex), true }},

        { 0x24a1ea07, { "sysPPUThreadCreate",                               std::bind(&SysThread::sysPPUThreadCreate, &sysThread), true }},
        { 0x350d454e, { "sysPPUThreadGetID",                                std::bind(&SysThread::sysPPUThreadGetID, &sysThread), true }},
        { 0x744680a2, { "sysPPUThreadInitializeTLS",                        std::bind(&SysThread::sysPPUThreadInitializeTLS, &sysThread), true }},
        { 0xa3e3be68, { "sysPPUThreadOnce",                                 std::bind(&SysThread::sysPPUThreadOnce, &sysThread), true }},
        { 0xaff080a4, { "sysPPUThreadExit",                                 std::bind(&SysThread::sysPPUThreadExit, &sysThread), true }},

        { 0x409ad939, { "sysMMapperFreeMemory",                             std::bind(&SysMMapper::sysMMapperFreeMemory, &sysMMapper) }},
        { 0x4643ba6e, { "sysMMapperUnmapMemory",                            std::bind(&SysMMapper::sysMMapperUnmapMemory, &sysMMapper) }},
        { 0xb257540b, { "sysMMapperAllocateMemory",                         std::bind(&SysMMapper::sysMMapperAllocateMemory, &sysMMapper) }},
        { 0xdc578057, { "sysMMapperMapMemory",                              std::bind(&SysMMapper::sysMMapperMapMemory, &sysMMapper) }},

        { 0x055bd74d, { "cellGcmGetTiledPitchSize",                         std::bind(&CellGcmSys::cellGcmGetTiledPitchSize, &cellGcmSys) }},
        { 0x06edea9e, { "cellGcmSetUserHandler",                            std::bind(&ModuleManager::stub, this) } },
        { 0x0a862772, { "cellGcmSetQueueHandler",                           std::bind(&CellGcmSys::cellGcmSetQueueHandler, &cellGcmSys) } },
        { 0x0e6b0dae, { "cellGcmGetDisplayInfo",                            std::bind(&CellGcmSys::cellGcmGetDisplayInfo, &cellGcmSys) }},
        { 0x15bae46b, { "cellGcmInitBody",                                  std::bind(&CellGcmSys::cellGcmInitBody, &cellGcmSys) }},
        { 0x21397818, { " _cellGcmSetFlipCommand",                          std::bind(&CellGcmSys::_cellGcmSetFlipCommand, &cellGcmSys) }},
        { 0x21ac3697, { "cellGcmAddressToOffset",                           std::bind(&CellGcmSys::cellGcmAddressToOffset, &cellGcmSys) }},
        { 0x23ae55a3, { "cellGcmGetLastSecondVTime",                        std::bind(&CellGcmSys::cellGcmGetLastSecondVTime, &cellGcmSys) }},
        { 0x2922aed0, { "cellGcmGetOffsetTable",                            std::bind(&CellGcmSys::cellGcmGetOffsetTable, &cellGcmSys) }},
        { 0x2a6fba9c, { "cellGcmIoOffsetToAddress",                         std::bind(&CellGcmSys::cellGcmIoOffsetToAddress, &cellGcmSys) }},
        { 0x4524cccd, { "cellGcmBindTile",                                  std::bind(&CellGcmSys::cellGcmBindTile, &cellGcmSys) }},
        { 0x4ae8d215, { "cellGcmSetFlipMode",                               std::bind(&CellGcmSys::cellGcmSetFlipMode, &cellGcmSys) }},
        { 0x4d7ce993, { "cellGcmSetSecondVFrequency",                       std::bind(&CellGcmSys::cellGcmSetSecondVFrequency, &cellGcmSys) }},
        { 0x51c9d62b, { "cellGcmSetDebugOutputLevel",                       std::bind(&CellGcmSys::cellGcmSetDebugOutputLevel, &cellGcmSys) }},
        { 0x5e2ee0f0, { "cellGcmGetDefaultCommandWordSize",                 std::bind(&CellGcmSys::cellGcmGetDefaultCommandWordSize, &cellGcmSys) }},
        { 0x5a41c10f, { "cellGcmGetTimeStamp",                              std::bind(&ModuleManager::stub, this) } },
        { 0x626e8518, { "cellGcmMapEaIoAddressWithFlags",                   std::bind(&CellGcmSys::cellGcmMapEaIoAddressWithFlags, &cellGcmSys) }},
        { 0x63441cb4, { "cellGcmMapEaIoAddress",                            std::bind(&CellGcmSys::cellGcmMapEaIoAddress, &cellGcmSys) }},
        { 0x63387071, { "cellGcmGetLastFlipTime",                           std::bind(&CellGcmSys::cellGcmGetLastFlipTime, &cellGcmSys) }},
        { 0x72a577ce, { "cellGcmGetFlipStatus",                             std::bind(&CellGcmSys::cellGcmGetFlipStatus, &cellGcmSys) }},
        { 0x8572bce2, { "cellGcmGetReportDataAddressLocation",              std::bind(&CellGcmSys::cellGcmGetReportDataAddressLocation, &cellGcmSys) }},
        { 0x8cdf8c70, { "cellGcmGetDefaultSegmentWordSize",                 std::bind(&CellGcmSys::cellGcmGetDefaultSegmentWordSize, &cellGcmSys) }},
        { 0x983fb9aa, { "cellGcmSetWaitFlip",                               std::bind(&CellGcmSys::cellGcmSetWaitFlip, &cellGcmSys) }},
        { 0x99d397ac, { "cellGcmGetReport",                                 std::bind(&ModuleManager::stub, this) } },
        { 0x9a0159af, { "cellGcmGetReportDataAddress",                      std::bind(&CellGcmSys::cellGcmGetReportDataAddress, &cellGcmSys) }},
        { 0x9ba451e4, { "cellGcmSetDefaultFifoSize",                        std::bind(&ModuleManager::stub, this) } },
        { 0x9dc04436, { "cellGcmBindZcull",                                 std::bind(&CellGcmSys::cellGcmBindZcull, &cellGcmSys) }},
        { 0xa114ec67, { "cellGcmMapMainMemory",                             std::bind(&CellGcmSys::cellGcmMapMainMemory, &cellGcmSys) }},
        { 0xa41ef7e8, { "cellGcmSetFlipHandler",                            std::bind(&CellGcmSys::cellGcmSetFlipHandler, &cellGcmSys) }},
        { 0xa53d12ae, { "cellGcmSetDisplayBuffer",                          std::bind(&CellGcmSys::cellGcmSetDisplayBuffer, &cellGcmSys) }},
        { 0xa547adde, { "cellGcmGetControlRegister",                        std::bind(&CellGcmSys::cellGcmGetControlRegister, &cellGcmSys) }},
        { 0xa6b180ac, { "cellGcmGetReportDataLocation",                     std::bind(&ModuleManager::stub, this) } },
        { 0xa75640e8, { "cellGcmUnbindZcull",                               std::bind(&ModuleManager::stub, this) } },
        { 0xa91b0402, { "cellGcmSetVBlankHandler",                          std::bind(&CellGcmSys::cellGcmSetVBlankHandler, &cellGcmSys) }},
        { 0xb2e761d4, { "cellGcmResetFlipStatus",                           std::bind(&CellGcmSys::cellGcmResetFlipStatus, &cellGcmSys) }},
        { 0xbc982946, { "cellGcmSetDefaultCommandBuffer",                   std::bind(&CellGcmSys::cellGcmSetDefaultCommandBuffer, &cellGcmSys) }},
        { 0xbd100dbc, { "cellGcmSetTileInfo",                               std::bind(&CellGcmSys::cellGcmSetTileInfo, &cellGcmSys) }},
        { 0xcaabd992, { "cellGcmInitDefaultFifoMode",                       std::bind(&CellGcmSys::cellGcmInitDefaultFifoMode, &cellGcmSys) }},
        { 0xd01b570d, { "cellGcmSetGraphicsHandler",                        std::bind(&CellGcmSys::cellGcmSetGraphicsHandler, &cellGcmSys) }},
        { 0xd0b1d189, { "cellGcmSetTile",                                   std::bind(&CellGcmSys::cellGcmSetTile, &cellGcmSys) }},
        { 0xd34a420d, { "cellGcmSetZcull",                                  std::bind(&CellGcmSys::cellGcmSetZcull, &cellGcmSys) }},
        { 0xd8f88e1a, { "_cellGcmSetFlipCommandWithWaitLabel",              std::bind(&CellGcmSys::cellGcmSetFlip, &cellGcmSys) }},  // TODO: I don't know what the "with wait label" part means
        { 0xd9b7653e, { "cellGcmUnbindTile",                                std::bind(&CellGcmSys::cellGcmUnbindTile, &cellGcmSys) }},
        { 0xdb23e867, { "cellGcmUnmapIoAddress",                            std::bind(&CellGcmSys::cellGcmUnmapIoAddress, &cellGcmSys) }},
        { 0xdc09357e, { "cellGcmSetFlip",                                   std::bind(&CellGcmSys::cellGcmSetFlip, &cellGcmSys) }},
        { 0xdc494430, { "cellGcmSetSecondVHandler",                         std::bind(&CellGcmSys::cellGcmSetSecondVHandler, &cellGcmSys) } },
        { 0xe315a0b2, { "cellGcmGetConfiguration",                          std::bind(&CellGcmSys::cellGcmGetConfiguration, &cellGcmSys) }},
        { 0xefd00f54, { "cellGcmUnmapEaIoAddress",                          std::bind(&CellGcmSys::cellGcmUnmapEaIoAddress, &cellGcmSys) }},
        { 0xf80196c1, { "cellGcmGetLabelAddress",                           std::bind(&CellGcmSys::cellGcmGetLabelAddress, &cellGcmSys) }},
        { 0xffe0160e, { "cellGcmSetVBlankFrequency",                        std::bind(&CellGcmSys::cellGcmSetVBlankFrequency, &cellGcmSys) }},

        { 0x0bae8772, { "cellVideoOutConfigure",                            std::bind(&CellVideoOut::cellVideoOutConfigure, &cellVideoOut) }},
        { 0x15b0b0cd, { "cellVideoOutGetConfiguration",                     std::bind(&CellVideoOut::cellVideoOutGetConfiguration, &cellVideoOut) }},
        { 0x1e930eef, { "cellVideoOutGetDeviceInfo",                        std::bind(&CellVideoOut::cellVideoOutGetDeviceInfo, &cellVideoOut) }},
        { 0x75bbb672, { "cellVideoOutGetNumberOfDevice",                    std::bind(&CellVideoOut::cellVideoOutGetNumberOfDevice, &cellVideoOut) }},
        { 0x887572d5, { "cellVideoOutGetState",                             std::bind(&CellVideoOut::cellVideoOutGetState, &cellVideoOut) }},
        { 0xa322db75, { "cellVideoOutGetResolutionAvailability",            std::bind(&CellVideoOut::cellVideoOutGetResolutionAvailability, &cellVideoOut) }},
        { 0xc7020f62, { "cellVideoOutSetGamma",                             std::bind(&ModuleManager::stub, this) }},
        { 0xe558748d, { "cellVideoOutGetResolution",                        std::bind(&CellVideoOut::cellVideoOutGetResolution, &cellVideoOut) }},

        { 0x02ff3c1b, { "cellSysutilUnregisterCallback",                    std::bind(&CellSysutil::cellSysutilUnregisterCallback, &cellSysutil) }},
        { 0x189a74da, { "cellSysutilCheckCallback",                         std::bind(&CellSysutil::cellSysutilCheckCallback, &cellSysutil) }},
        { 0x220894e3, { "cellSysutilEnableBgmPlayback",                     std::bind(&ModuleManager::stub, this) }},
        { 0x2f280883, { "cellSysutilAvc2EstimateMemoryContainerSize",       std::bind(&ModuleManager::stub, this) }},
        { 0x40e895d3, { "cellSysutilGetSystemParamInt",                     std::bind(&CellSysutil::cellSysutilGetSystemParamInt, &cellSysutil) }},
        { 0x571dc686, { "cellSysutilGetLicenseArea",                        std::bind(&ModuleManager::stub, this) }},
        { 0x6cfd856f, { "cellSysutilGetBgmPlaybackStatus2",                 std::bind(&ModuleManager::stub, this) }},
        { 0x89456724, { "cellSysutilAvc2InitParam",                         std::bind(&ModuleManager::stub, this) }},
        { 0x938013a0, { "cellSysutilGetSystemParamString",                  std::bind(&CellSysutil::cellSysutilGetSystemParamString, &cellSysutil) }},
        { 0x9d98afa0, { "cellSysutilRegisterCallback",                      std::bind(&CellSysutil::cellSysutilRegisterCallback, &cellSysutil) }},
        { 0xa11552f6, { "cellSysutilGetBgmPlaybackStatus",                  std::bind(&ModuleManager::stub, this) }},
        { 0xcfdd8e87, { "cellSysutilDisableBgmPlayback",                    std::bind(&ModuleManager::stub, this) }},

        { 0x112a5ee9, { "cellSysmoduleUnloadModule",                        std::bind(&CellSysmodule::cellSysmoduleUnloadModule, &cellSysmodule) }},
        { 0x32267a31, { "cellSysmoduleLoadModule",                          std::bind(&CellSysmodule::cellSysmoduleLoadModule, &cellSysmodule) }},
        { 0x5a59e258, { "cellSysmoduleIsLoaded",                            std::bind(&ModuleManager::stub, this) }},
        { 0x63ff6ff9, { "cellSysmoduleInitialize",                          std::bind(&ModuleManager::stub, this) }},
        { 0xa193143c, { "cellSysmoduleSetMemcontainer",                     std::bind(&ModuleManager::stub, this) }},

        { 0x01220224, { "cellRescGcmSurface2RescSrc",                       std::bind(&ModuleManager::stub, this) }},
        { 0x0d3c22ce, { "cellRescSetWaitFlip",                              std::bind(&CellResc::cellRescSetWaitFlip, &cellResc) }},
        { 0x10db5b1a, { "cellRescSetDsts",                                  std::bind(&CellResc::cellRescSetDsts, &cellResc) }},
        { 0x129922a0, { "cellRescResetFlipStatus",                          std::bind(&CellResc::cellRescResetFlipStatus, &cellResc) }},
        { 0x1dd3c4cd, { "cellRescGetRegisterCount",                         std::bind(&ModuleManager::stub, this) }},
        { 0x23134710, { "cellRescSetDisplayMode",                           std::bind(&CellResc::cellRescSetDisplayMode, &cellResc) }},
        { 0x25c107e6, { "cellRescSetConvertAndFlip",                        std::bind(&CellResc::cellRescSetConvertAndFlip, &cellResc), true }},
        { 0x516ee89e, { "cellRescInit",                                     std::bind(&CellResc::cellRescInit, &cellResc) }},
        { 0x5a338cdb, { "cellRescGetBufferSize",                            std::bind(&CellResc::cellRescGetBufferSize, &cellResc) }},
        { 0x6cd0f95f, { "cellRescSetSrc",                                   std::bind(&ModuleManager::stub, this) }},
        { 0x8107277c, { "cellRescSetBufferAddress",                         std::bind(&CellResc::cellRescSetBufferAddress, &cellResc) }},
        { 0xc47c5c22, { "cellRescGetFlipStatus",                            std::bind(&CellResc::cellRescGetFlipStatus, &cellResc) }},
        { 0xd1ca0503, { "cellRescVideoOutResolutionId2RescBufferMode",      std::bind(&CellResc::cellRescVideoOutResolutionId2RescBufferMode, &cellResc) }},

        { 0x042e74e3, { "cellFontCreateRenderer",                           std::bind(&ModuleManager::stub, this) }},
        { 0x073fa321, { "cellFontOpenFontsetOnMemory",                      std::bind(&ModuleManager::stub, this) }},
        { 0x1387c45c, { "cellFontGetHorizontalLayout",                      std::bind(&ModuleManager::stub, this) }},
        { 0x227e1e3c, { "cellFontSetupRenderScalePixel",                    std::bind(&ModuleManager::stub, this) }},
        { 0x25253fe4, { "cellFontSetEffectWeight",                          std::bind(&ModuleManager::stub, this) }},
        { 0x29329541, { "cellFontOpenFontInstance",                         std::bind(&ModuleManager::stub, this) }},
        { 0x297f0e93, { "cellFontSetScalePixel",                            std::bind(&ModuleManager::stub, this) }},
        { 0x66a23100, { "cellFontBindRenderer",                             std::bind(&ModuleManager::stub, this) }},
        { 0x698897f8, { "cellFontGetVerticalLayout",                        std::bind(&ModuleManager::stub, this) }},
        { 0x70f3e728, { "cellFontSetScalePoint",                            std::bind(&ModuleManager::stub, this) }},
        { 0x78d05e08, { "cellFontSetupRenderEffectSlant",                   std::bind(&ModuleManager::stub, this) }},
        { 0x7a0a83c4, { "cellFontInitLibraryFreeTypeWithRevision",          std::bind(&ModuleManager::stub, this) }},
        { 0x8657c8f5, { "cellFontSetEffectSlant",                           std::bind(&ModuleManager::stub, this) }},
        { 0x88be4799, { "cellFontRenderCharGlyphImage",                     std::bind(&ModuleManager::stub, this) }},
        { 0x90b9465e, { "cellFontRenderSurfaceInit",                        std::bind(&ModuleManager::stub, this) }},
        { 0x9e19072b, { "cellFontOpenFontMemory",                           std::bind(&ModuleManager::stub, this) }},
        { 0xa6dc25d1, { "cellFontSetupRenderEffectWeight",                  std::bind(&ModuleManager::stub, this) }},
        { 0xa885cc9b, { "cellFontOpenFontset",                              std::bind(&ModuleManager::stub, this) }},
        { 0xb422b005, { "cellFontRenderSurfaceSetScissor",                  std::bind(&ModuleManager::stub, this) }},
        { 0xd8eaee9f, { "cellFontGetCharGlyphMetrics",                      std::bind(&ModuleManager::stub, this) }},
        { 0xf03dcc29, { "cellFontInitializeWithRevision",                   std::bind(&ModuleManager::stub, this) }},
        { 0xfb3341ba, { "cellFontSetResolutionDpi",                         std::bind(&ModuleManager::stub, this) }},

        { 0x3a5d726a, { "cellGameGetParamString",                           std::bind(&CellGame::cellGameGetParamString, &cellGame) }},
        { 0x2a8e6b92, { "cellGameGetDiscContentInfoUpdatePath",             std::bind(&ModuleManager::stub, this) }},
        { 0x70acec67, { "cellGameContentPermit",                            std::bind(&CellGame::cellGameContentPermit, &cellGame) }},
        { 0xb0a1f8c6, { "cellGameContentErrorDialog",                       std::bind(&CellGame::cellGameContentErrorDialog, &cellGame) }},
        { 0xb7a45caf, { "cellGameGetParamInt",                              std::bind(&CellGame::cellGameGetParamInt, &cellGame) }},
        { 0xc9645c41, { "cellGameDataCheckCreate2",                         std::bind(&CellGame::cellGameDataCheckCreate2, &cellGame) } },
        { 0xce4374f6, { "cellGamePatchCheck",                               std::bind(&CellGame::cellGamePatchCheck, &cellGame) }},
        { 0xdb9819f3, { "cellGameDataCheck",                                std::bind(&CellGame::cellGameDataCheck, &cellGame) }},
        { 0xf52639ea, { "cellGameBootCheck",                                std::bind(&CellGame::cellGameBootCheck, &cellGame) }},
        
        { 0x9117df20, { "cellHddGameCheck",                                 std::bind(&CellGame::cellHddGameCheck, &cellGame) } },

        { 0x011ee38b, { "_cellSpursLFQueueInitialize",                      std::bind(&CellSpurs::_cellSpursLFQueueInitialize, &cellSpurs) }},
        { 0x07529113, { "cellSpursAttributeSetNamePrefix",                  std::bind(&CellSpurs::cellSpursAttributeSetNamePrefix, &cellSpurs) }},
        { 0x1051d134, { "cellSpursAttributeEnableSpuPrintfIfAvailable",     std::bind(&CellSpurs::cellSpursAttributeEnableSpuPrintfIfAvailable, &cellSpurs) }},
        { 0x16394a4e, { "_cellSpursTasksetAttributeInitialize",             std::bind(&CellSpurs::_cellSpursTasksetAttributeInitialize, &cellSpurs) }},
        { 0x1656d49f, { "cellSpursLFQueueAttachLv2EventQueue",              std::bind(&CellSpurs::cellSpursLFQueueAttachLv2EventQueue, &cellSpurs) }},
        { 0x182d9890, { "cellSpursRequestIdleSpu",                          std::bind(&CellSpurs::cellSpursRequestIdleSpu, &cellSpurs) }},
        { 0x1d46fedf, { "cellSpursCreateTaskWithAttribute",                 std::bind(&CellSpurs::cellSpursCreateTaskWithAttribute, &cellSpurs) }},
        { 0x1f402f8f, { "cellSpursGetInfo",                                 std::bind(&CellSpurs::cellSpursGetInfo, &cellSpurs) }},
        { 0x30aa96c4, { "cellSpursInitializeWithAttribute2",                std::bind(&CellSpurs::cellSpursInitializeWithAttribute2, &cellSpurs) }},
        { 0x373523d4, { "cellSpursEventFlagWait",                           std::bind(&ModuleManager::stub, this) } },
        { 0x4a5eab63, { "cellSpursWorkloadAttributeSetName",                std::bind(&CellSpurs::cellSpursWorkloadAttributeSetName, &cellSpurs) }},
        { 0x4a6465e3, { "cellSpursCreateTaskset2",                          std::bind(&CellSpurs::cellSpursCreateTaskset2, &cellSpurs) }},
        { 0x4ac7bae4, { "cellSpursEventFlagClear",                          std::bind(&CellSpurs::cellSpursEventFlagClear, &cellSpurs) }},
        { 0x52cc6c82, { "cellSpursCreateTaskset",                           std::bind(&CellSpurs::cellSpursCreateTaskset, &cellSpurs) }},
        { 0x5ef96465, { "_cellSpursEventFlagInitialize",                    std::bind(&CellSpurs::_cellSpursEventFlagInitialize, &cellSpurs) }},
        { 0x652b70e2, { "cellSpursTasksetAttributeSetName",                 std::bind(&CellSpurs::cellSpursTasksetAttributeSetName, &cellSpurs) }},
        { 0x82275c1c, { "cellSpursAttributeSetMemoryContainerForSpuThread", std::bind(&CellSpurs::cellSpursAttributeSetMemoryContainerForSpuThread, &cellSpurs) }},
        { 0x87630976, { "cellSpursEventFlagAttachLv2EventQueue",            std::bind(&CellSpurs::cellSpursEventFlagAttachLv2EventQueue, &cellSpurs) }},
        { 0x8a85674d, { "_cellSpursLFQueuePushBody",                        std::bind(&CellSpurs::_cellSpursLFQueuePushBody, &cellSpurs) }},
        { 0x95180230, { "_cellSpursAttributeInitialize",                    std::bind(&CellSpurs::_cellSpursAttributeInitialize, &cellSpurs) }},
        { 0x9dcbcb5d, { "cellSpursAttributeEnableSystemWorkload",           std::bind(&CellSpurs::cellSpursAttributeEnableSystemWorkload, &cellSpurs) }},
        { 0xa73bf47e, { "_cellSpursWorkloadFlagReceiver",                   std::bind(&CellSpurs::_cellSpursWorkloadFlagReceiver, &cellSpurs) }},
        { 0xa839a4d9, { "cellSpursAttributeSetSpuThreadGroupType",          std::bind(&CellSpurs::cellSpursAttributeSetSpuThreadGroupType, &cellSpurs) }},
        { 0xaa6269a8, { "cellSpursInitializeWithAttribute",                 std::bind(&CellSpurs::cellSpursInitializeWithAttribute, &cellSpurs) }},
        { 0xacfc8dbc, { "cellSpursInitialize",                              std::bind(&CellSpurs::cellSpursInitialize, &cellSpurs) }},
        { 0xb8474eff, { "_cellSpursTaskAttributeInitialize",                std::bind(&CellSpurs::_cellSpursTaskAttributeInitialize, &cellSpurs) }},
        { 0xb9bc6207, { "cellSpursAttachLv2EventQueue",                     std::bind(&CellSpurs::cellSpursAttachLv2EventQueue, &cellSpurs) }},
        { 0xbeb600ac, { "cellSpursCreateTask",                              std::bind(&CellSpurs::cellSpursCreateTask, &cellSpurs) }},
        { 0xc0158d8b, { "cellSpursAddWorkloadWithAttribute",                std::bind(&CellSpurs::cellSpursAddWorkloadWithAttribute, &cellSpurs) }},
        { 0xc10931cb, { "cellSpursCreateTasksetWithAttribute",              std::bind(&CellSpurs::cellSpursCreateTasksetWithAttribute, &cellSpurs) }},
        { 0xc2acdf43, { "_cellSpursTasksetAttribute2Initialize",            std::bind(&CellSpurs::_cellSpursTasksetAttribute2Initialize, &cellSpurs) }},
        { 0xc765b995, { "cellSpursGetWorkloadFlag",                         std::bind(&CellSpurs::cellSpursGetWorkloadFlag, &cellSpurs) }},
        { 0xd2e23fa9, { "cellSpursSetExceptionEventHandler",                std::bind(&CellSpurs::cellSpursSetExceptionEventHandler, &cellSpurs) }},
        { 0xe0a6dbe4, { "_cellSpursSendSignal",                             std::bind(&ModuleManager::stub, this) } },
        { 0xefeb2679, { "_cellSpursWorkloadAttributeInitialize",            std::bind(&CellSpurs::_cellSpursWorkloadAttributeInitialize, &cellSpurs) }},
        { 0xf5507729, { "cellSpursEventFlagSet",                            std::bind(&ModuleManager::stub, this) } },

        { 0x2cce9cf5, { "cellRtcGetCurrentClockLocalTime",                  std::bind(&CellRtc::cellRtcGetCurrentClockLocalTime, &cellRtc) }},
        { 0x9dafc0d9, { "cellRtcGetCurrentTick",                            std::bind(&CellRtc::cellRtcGetCurrentTick, &cellRtc) }},
        { 0x99b13034, { "cellRtcSetTick",                                   std::bind(&ModuleManager::stub, this) } },
        { 0xbb543189, { "cellRtcSetTime_t",                                 std::bind(&ModuleManager::stub, this) } },
        { 0xc7bdb7eb, { "cellRtcGetTick",                                   std::bind(&ModuleManager::stub, this) } },
        { 0xcb90c761, { "cellRtcGetTime_t",                                 std::bind(&CellRtc::cellRtcGetTime_t, &cellRtc) }},

        { 0x0d5b4a14, { "cellFsReadWithOffset",                             std::bind(&CellFs::cellFsReadWithOffset, &cellFs) }},
        { 0x2cb51f0d, { "cellFsClose",                                      std::bind(&CellFs::cellFsClose, &cellFs) }},
        { 0x3f61245c, { "cellFsOpendir",                                    std::bind(&CellFs::cellFsOpendir, &cellFs) }},
        { 0x4d5ff8e2, { "cellFsRead",                                       std::bind(&CellFs::cellFsRead, &cellFs) }},
        { 0x5c74903d, { "cellFsReaddir",                                    std::bind(&CellFs::cellFsReaddir, &cellFs) }},
        { 0x718bf5f8, { "cellFsOpen",                                       std::bind(&CellFs::cellFsOpen, &cellFs) }},
        { 0x7de6dced, { "cellFsStat",                                       std::bind(&CellFs::cellFsStat, &cellFs) }},
        { 0x7f4677a8, { "cellFsUnlink",                                     std::bind(&ModuleManager::stub, this) } },
        { 0x9b882495, { "cellFsGetDirectoryEntries",                        std::bind(&CellFs::cellFsGetDirectoryEntries, &cellFs) }},
        { 0xa397d042, { "cellFsLseek",                                      std::bind(&CellFs::cellFsLseek, &cellFs) }},
        { 0xaa3b4bcd, { "cellFsGetFreeSize",                                std::bind(&CellFs::cellFsGetFreeSize, &cellFs) }},
        { 0xb1840b53, { "cellFsSdataOpen",                                  std::bind(&CellFs::cellFsSdataOpen, &cellFs) }},
        { 0xba901fe6, { "cellFsMkdir",                                      std::bind(&CellFs::cellFsMkdir, &cellFs) }},
        { 0xdb869f20, { "cellFsAioInit",                                    std::bind(&ModuleManager::stub, this) }},
        { 0xef3efa34, { "cellFsFstat",                                      std::bind(&CellFs::cellFsFstat, &cellFs) }},
        { 0xff42dcc3, { "cellFsClosedir",                                   std::bind(&CellFs::cellFsClosedir, &cellFs) }},

        { 0x04af134e, { "cellAudioCreateNotifyEventQueue",                  std::bind(&CellAudio::cellAudioCreateNotifyEventQueue, &cellAudio) }},
        { 0x0b168f92, { "cellAudioInit",                                    std::bind(&CellAudio::cellAudioInit, &cellAudio) }},
        { 0x377e0cd9, { "cellAudioSetNotifyEventQueue",                     std::bind(&CellAudio::cellAudioSetNotifyEventQueue, &cellAudio) }},
        { 0x74a66af0, { "cellAudioGetPortConfig",                           std::bind(&CellAudio::cellAudioGetPortConfig, &cellAudio) }},
        { 0x56dfe179, { "cellAudioSetPortLevel",                            std::bind(&ModuleManager::stub, this) } },
        { 0xca5ac370, { "cellAudioQuit",                                    std::bind(&ModuleManager::stub, this) } },
        { 0xdab029aa, { "cellAudioAddData",                                 std::bind(&CellAudio::cellAudioAddData, &cellAudio) }},
        { 0xff3626fd, { "cellAudioRemoveNotifyEventQueue",                  std::bind(&ModuleManager::stub, this) } },
        
        { 0x4129fe2d, { "cellAudioPortClose",                               std::bind(&CellAudio::cellAudioPortClose, &cellAudio) }},
        { 0x5b1e2c73, { "cellAudioPortStop",                                std::bind(&ModuleManager::stub, this) } },
        { 0x89be28f2, { "cellAudioPortStart",                               std::bind(&CellAudio::cellAudioPortStart, &cellAudio) }},
        { 0xcd7bc431, { "cellAudioPortOpen",                                std::bind(&CellAudio::cellAudioPortOpen, &cellAudio) }},

        { 0xeb6c50fb, { "cellAudioInSetDeviceMode",                         std::bind(&ModuleManager::stub, this) } },

        { 0x2beac488, { "cellAudioOutGetSoundAvailability2",                std::bind(&ModuleManager::stub, this) }},
        { 0x4692ab35, { "cellAudioOutConfigure",                            std::bind(&ModuleManager::stub, this) }},
        { 0xc01b4e7c, { "cellAudioOutGetSoundAvailability",                 std::bind(&CellAudioOut::cellAudioOutGetSoundAvailability, &cellAudioOut) }},
        { 0xc96e89e9, { "cellAudioOutSetCopyControl",                       std::bind(&ModuleManager::stub, this) }},
        { 0xe5e2b09d, { "cellAudioOutGetNumberOfDevice",                    std::bind(&ModuleManager::stub, this) }},
        { 0xed5d96af, { "cellAudioOutGetConfiguration",                     std::bind(&ModuleManager::stub, this) }},
        { 0xf4e3caa0, { "cellAudioOutGetState",                             std::bind(&CellAudioOut::cellAudioOutGetState, &cellAudioOut) }},

        { 0x0d5f2c14, { "cellPadClearBuf",                                  std::bind(&ModuleManager::stub, this) } },
        { 0x0e2dfaad, { "cellPadInfoPressMode",                             std::bind(&ModuleManager::stub, this) } },
        { 0x1cf98800, { "cellPadInit",                                      std::bind(&CellPad::cellPadInit, &cellPad) }},
        { 0x3aaad464, { "cellPadGetInfo",                                   std::bind(&CellPad::cellPadGetInfo, &cellPad)}},
        { 0x578e3c98, { "cellPadSetPortSetting",                            std::bind(&ModuleManager::stub, this) }},
        { 0x78200559, { "cellPadInfoSensorMode",                            std::bind(&ModuleManager::stub, this) }},
        { 0x8b72cda1, { "cellPadGetData",                                   std::bind(&CellPad::cellPadGetData, &cellPad)}},
        { 0xa703a51d, { "cellPadGetInfo2",                                  std::bind(&CellPad::cellPadGetInfo2, &cellPad)}},
        { 0xbe5be3ba, { "cellPadSetSensorMode",                             std::bind(&ModuleManager::stub, this) } },
        { 0xdbf4c59c, { "cellPadGetCapabilityInfo",                         std::bind(&ModuleManager::stub, this) } },
        { 0xf65544ee, { "cellPadSetActDirect",                              std::bind(&ModuleManager::stub, this) } },
        { 0xf83f8182, { "cellPadSetPressMode",                              std::bind(&ModuleManager::stub, this) } },

        { 0x2f1774d5, { "cellKbGetInfo",                                    std::bind(&CellKb::cellKbGetInfo, &cellKb) }},
        { 0xff0a21b7, { "cellKbRead",                                       std::bind(&CellKb::cellKbRead, &cellKb) }},
        
        { 0x3138e632, { "cellMouseGetData",                                 std::bind(&ModuleManager::stub, this) } },
        { 0x3ef66b95, { "cellMouseClearBuf",                                std::bind(&ModuleManager::stub, this) } },
        { 0x5baf30fb, { "cellMouseGetInfo",                                 std::bind(&ModuleManager::stub, this) } },
        { 0xc9030138, { "cellMouseInit",                                    std::bind(&ModuleManager::stub, this) } },

        { 0x7903400e, { "cellMicSetNotifyEventQueue",                       std::bind(&ModuleManager::stub, this) } },
        { 0x8325e02d, { "cellMicInit",                                      std::bind(&ModuleManager::stub, this) } },

        { 0x168fcece, { "sceNpManagerGetAccountAge",                        std::bind(&ModuleManager::stub, this) }},
        { 0x2ecd48ed, { "sceNpDrmVerifyUpgradeLicense",                     std::bind(&SceNp::sceNpDrmVerifyUpgradeLicense, &sceNp) }},
        { 0x32cf311f, { "sceNpScoreInit",                                   std::bind(&ModuleManager::stub, this) }},
        { 0x3539d233, { "sceNpCommerce2Init",                               std::bind(&ModuleManager::stub, this) }},
        { 0x4026eac5, { "sceNpBasicRegisterContextSensitiveHandler",        std::bind(&ModuleManager::stub, this) }},
        { 0x45f8f3aa, { "sceNpCustomMenuRegisterActions",                   std::bind(&ModuleManager::stub, this) }},
        { 0x4885aa18, { "sceNpTerm",                                        std::bind(&ModuleManager::stub, this) }},
        { 0x4b9efb7a, { "sceNpManagerGetCachedInfo",                        std::bind(&ModuleManager::stub, this) }},
        { 0x52a6b523, { "sceNpManagerUnregisterCallback",                   std::bind(&ModuleManager::stub, this) }},
        { 0x5e849303, { "sceNpBasicSetPresenceDetails2",                    std::bind(&ModuleManager::stub, this) }},
        { 0x5f2d9257, { "sceNpLookupInit",                                  std::bind(&ModuleManager::stub, this) }},
        { 0x6ee62ed2, { "sceNpManagerGetContentRatingFlag",                 std::bind(&ModuleManager::stub, this) }},
        { 0x73931bd0, { "sceNpBasicGetBlockListEntryCount",                 std::bind(&ModuleManager::stub, this) }},
        { 0x9851f805, { "sceNpScoreTerm",                                   std::bind(&ModuleManager::stub, this) }},
        { 0xa7bff757, { "sceNpManagerGetStatus",                            std::bind(&SceNp::sceNpManagerGetStatus, &sceNp) }},
        { 0xad218faf, { "sceNpDrmIsAvailable",                              std::bind(&ModuleManager::stub, this) }},
        { 0xafef640d, { "sceNpBasicGetFriendListEntryCount",                std::bind(&ModuleManager::stub, this) }},
        { 0xb1e0718b, { "sceNpManagerGetAccountRegion",                     std::bind(&ModuleManager::stub, this) }},
        { 0xbcc09fe7, { "sceNpBasicRegisterHandler",                        std::bind(&ModuleManager::stub, this) }},
        { 0xbd28fdbf, { "sceNpInit",                                        std::bind(&ModuleManager::stub, this) }},
        { 0xbe07c708, { "sceNpManagerGetOnlineId",                          std::bind(&ModuleManager::stub, this) }},
        { 0xbe0e3ee2, { "sceNpDrmVerifyUpgradeLicense2",                    std::bind(&ModuleManager::stub, this) }},
        { 0xe035f7d6, { "sceNpBasicGetEvent",                               std::bind(&SceNp::sceNpBasicGetEvent, &sceNp) } },
        { 0xe7dcd3b4, { "sceNpManagerRegisterCallback",                     std::bind(&ModuleManager::stub, this) }},
        { 0xeb7a3d84, { "sceNpManagerGetChatRestrictionFlag",               std::bind(&ModuleManager::stub, this) }},
        { 0xf042b14f, { "sceNpDrmIsAvailable2",                             std::bind(&ModuleManager::stub, this) }},
        { 0xfe37a7f4, { "sceNpManagerGetNpId",                              std::bind(&SceNp::sceNpManagerGetNpId, &sceNp) }},
        
        { 0x215b0d75, { "sceNpMatching2SetRoomDataExternal",                std::bind(&ModuleManager::stub, this) }},
        { 0x3f62c759, { "sceNpMatching2Init",                               std::bind(&ModuleManager::stub, this) }},
        { 0x6ba4c668, { "sceNpMatching2ContextStartAsync",                  std::bind(&ModuleManager::stub, this) }},
        { 0x748029a2, { "sceNpMatching2RegisterContextCallback",            std::bind(&ModuleManager::stub, this) }},
        { 0x8e5cfe9f, { "sceNpMatching2GetServerIdListLocal",               std::bind(&ModuleManager::stub, this) }},
        { 0x9cbce3f2, { "sceNpMatching2CreateContext",                      std::bind(&ModuleManager::stub, this) }},
        { 0xf4babd3f, { "sceNpMatching2Init2",                              std::bind(&ModuleManager::stub, this) }},

        { 0x41251f74, { "sceNp2Init",                                       std::bind(&ModuleManager::stub, this) }},
        { 0xaadb7c12, { "sceNp2Term",                                       std::bind(&ModuleManager::stub, this) }},
        
        { 0x8f87a06b, { "sceNpTusInit",                                     std::bind(&ModuleManager::stub, this) }},
        
        { 0x2c0f3548, { "sceNpSnsFbInit",                                   std::bind(&ModuleManager::stub, this) }},
        { 0x8fd1d549, { "sceNpSnsFbCreateHandle",                           std::bind(&ModuleManager::stub, this) }},

        { 0x04459230, { "cellNetCtlNetStartDialogLoadAsync",                std::bind(&ModuleManager::stub, this) }},
        { 0x0ce13c6b, { "cellNetCtlAddHandler",                             std::bind(&ModuleManager::stub, this) }},
        { 0x105ee2cb, { "cellNetCtlTerm",                                   std::bind(&ModuleManager::stub, this) }},
        { 0x1e585b5d, { "cellNetCtlGetInfo",                                std::bind(&ModuleManager::stub, this) }},
        { 0x8b3eba69, { "cellNetCtlGetState",                               std::bind(&CellNetCtl::cellNetCtlGetState, &cellNetCtl) } },
        { 0xbd5a59fc, { "cellNetCtlInit",                                   std::bind(&ModuleManager::stub, this) }},

        { 0x157d30c5, { "cellPngDecCreate",                                 std::bind(&CellPngDec::cellPngDecCreate, &cellPngDec) }},
        { 0x2310f155, { "cellPngDecDecodeData",                             std::bind(&CellPngDec::cellPngDecDecodeData, &cellPngDec) }},
        { 0x5b3d1ff1, { "cellPngDecClose",                                  std::bind(&CellPngDec::cellPngDecClose, &cellPngDec) }},
        { 0x820dae1a, { "cellPngDecDestroy",                                std::bind(&CellPngDec::cellPngDecDestroy, &cellPngDec) }},
        { 0x9ccdcc95, { "cellPngDecReadHeader",                             std::bind(&CellPngDec::cellPngDecReadHeader, &cellPngDec) }},
        { 0xd2bc5bfd, { "cellPngDecOpen",                                   std::bind(&CellPngDec::cellPngDecOpen, &cellPngDec) }},
        { 0xe97c9bd4, { "cellPngDecSetParameter",                           std::bind(&CellPngDec::cellPngDecSetParameter, &cellPngDec) }},

        { 0x6d9ebccf, { "cellJpgDecReadHeader",                             std::bind(&ModuleManager::stub, this) } },
        { 0x8b300f66, { "cellJpgDecExtCreate",                              std::bind(&ModuleManager::stub, this) } },
        { 0x9338a07a, { "cellJpgDecClose",                                  std::bind(&ModuleManager::stub, this) } },
        { 0x976ca5c2, { "cellJpgDecOpen",                                   std::bind(&ModuleManager::stub, this) } },
        { 0xa7978f59, { "cellJpgDecCreate",                                 std::bind(&ModuleManager::stub, this) } },
        { 0xaf8bb012, { "cellJpgDecDecodeData",                             std::bind(&ModuleManager::stub, this) } },
        { 0xd8ea91f8, { "cellJpgDecDestroy",                                std::bind(&ModuleManager::stub, this) } },
        { 0xe08f3910, { "cellJpgDecSetParameter",                           std::bind(&ModuleManager::stub, this) } },

        { 0x1197b52c, { "sceNpTrophyRegisterContext",                       std::bind(&SceNpTrophy::sceNpTrophyRegisterContext, &sceNpTrophy) } },
        { 0x1c25470d, { "sceNpTrophyCreateHandle",                          std::bind(&SceNpTrophy::sceNpTrophyCreateHandle, &sceNpTrophy) } },
        { 0x27deda93, { "sceNpTrophySetSoundLevel",                         std::bind(&ModuleManager::stub, this) } },
        { 0x370136fe, { "sceNpTrophyGetRequiredDiskSpace",                  std::bind(&SceNpTrophy::sceNpTrophyGetRequiredDiskSpace, &sceNpTrophy) } },
        { 0x39567781, { "sceNpTrophyInit",                                  std::bind(&ModuleManager::stub, this) } },
        { 0x49d18217, { "sceNpTrophyGetGameInfo",                           std::bind(&SceNpTrophy::sceNpTrophyGetGameInfo, &sceNpTrophy) } },
        { 0x623cd2dc, { "sceNpTrophyDestroyHandle",                         std::bind(&ModuleManager::stub, this) } },
        { 0x8ceedd21, { "sceNpTrophyUnlockTrophy",                          std::bind(&ModuleManager::stub, this) } },
        { 0xb3ac3478, { "sceNpTrophyGetTrophyUnlockState",                  std::bind(&SceNpTrophy::sceNpTrophyGetTrophyUnlockState, &sceNpTrophy) } },
        { 0xe3bf9a28, { "sceNpTrophyCreateContext",                         std::bind(&SceNpTrophy::sceNpTrophyCreateContext, &sceNpTrophy) } },

        { 0x21425307, { "cellSaveDataListAutoLoad",                         std::bind(&ModuleManager::stub, this) } },
        { 0x248bd1d8, { "cellSaveDataUserListAutoLoad",                     std::bind(&CellSaveData::cellSaveDataUserListAutoLoad, &cellSaveData) } },
        { 0x52aac4fa, { "cellSaveDataUserAutoSave",                         std::bind(&CellSaveData::cellSaveDataUserAutoSave, &cellSaveData) } },
        { 0x8b7ed64b, { "cellSaveDataAutoSave2",                            std::bind(&CellSaveData::cellSaveDataAutoSave2, &cellSaveData) } },
        { 0xcdc6aefd, { "cellSaveDataUserAutoLoad",                         std::bind(&CellSaveData::cellSaveDataUserAutoLoad, &cellSaveData) } },
        { 0xe7fa820b, { "cellSaveDataEnableOverlay",                        std::bind(&ModuleManager::stub, this) } },
        { 0xfbd5c856, { "cellSaveDataAutoLoad2",                            std::bind(&CellSaveData::cellSaveDataAutoLoad2, &cellSaveData) } },

        { 0x1c9a942c, { "sysLwCondDestroy",                                 std::bind(&ModuleManager::stub, this), true } },
        { 0x2a6d9d51, { "sysLwCondWait",                                    std::bind(&SysLwCond::sysLwCondWait, &sysLwCond), true } },
        { 0xda0eb71a, { "sysLwCondCreate",                                  std::bind(&SysLwCond::sysLwCondCreate, &sysLwCond), true } },
        { 0xe9a1bd84, { "sysLwCondSignalAll",                               std::bind(&SysLwCond::sysLwCondSignalAll, &sysLwCond), true } },
        { 0xef87a695, { "sysLwCondSignal",                                  std::bind(&SysLwCond::sysLwCondSignal, &sysLwCond), true } },

        { 0x1f71ecbe, { "cellKbGetConfiguration",                           std::bind(&ModuleManager::stub, this) } },
        { 0x433f6ec0, { "cellKbInit",                                       std::bind(&ModuleManager::stub, this) } },
        { 0x4ab1fa77, { "cellKbCnvRawCode",                                 std::bind(&ModuleManager::stub, this) } },
        { 0xa5f85e4d, { "cellKbSetCodeType",                                std::bind(&ModuleManager::stub, this) } },
        { 0xdeefdfa7, { "cellKbSetReadMode",                                std::bind(&ModuleManager::stub, this) } },

        { 0x139a9e9b, { "sysNetInitializeNetworkEx",                        std::bind(&ModuleManager::stub, this) } },

        { 0x91f2b7b0, { "cellSyncMutexUnlock",                              std::bind(&ModuleManager::stub, this) } },
        { 0xa9072dee, { "cellSyncMutexInitialize",                          std::bind(&ModuleManager::stub, this) } },
        { 0xd06918c4, { "cellSyncMutexTryLock",                             std::bind(&ModuleManager::stub, this) } },

        { 0x42b23552, { "sysPRXRegisterLibrary",                            std::bind(&ModuleManager::stub, this) } },
        { 0x84bb6774, { "sysPRXGetModuleInfo",                              std::bind(&ModuleManager::stub, this), true } },
        { 0xa5d06bf0, { "sysPRXGetModuleList",                              std::bind(&ModuleManager::stub, this), true } },

        { 0xb72bc4e6, { "cellDiscGameGetBootDiscInfo",                      std::bind(&CellGame::cellDiscGameGetBootDiscInfo, &cellGame) } },
        { 0xdfdd302e, { "cellDiscGameRegisterDiscChangeCallback",           std::bind(&ModuleManager::stub, this) } },

        { 0x0c4cb439, { "cellSailFutureReset",                              std::bind(&ModuleManager::stub, this) } },
        { 0x1139a206, { "cellSailPlayerSetSoundAdapter",                    std::bind(&ModuleManager::stub, this) } },
        { 0x18bcd21b, { "cellSailPlayerSetGraphicsAdapter",                 std::bind(&ModuleManager::stub, this) } },
        { 0x1c983864, { "cellSailGraphicsAdapterInitialize",                std::bind(&ModuleManager::stub, this) } },
        { 0x1c9d5e5a, { "cellSailSoundAdapterSetPreferredFormat",           std::bind(&ModuleManager::stub, this) } },
        { 0x23654375, { "cellSailPlayerInitialize2",                        std::bind(&ModuleManager::stub, this) } },
        { 0x2e3ccb5e, { "cellSailGraphicsAdapterSetPreferredFormat",        std::bind(&ModuleManager::stub, this) } },
        { 0x346ebba3, { "cellSailMemAllocatorInitialize",                   std::bind(&ModuleManager::stub, this) } },
        { 0x3a2d806c, { "cellSailFutureGet",                                std::bind(&ModuleManager::stub, this) } },
        { 0x3d0d3b72, { "cellSailSoundAdapterInitialize",                   std::bind(&ModuleManager::stub, this) } },
        { 0x4cc54f8e, { "cellSailFutureInitialize",                         std::bind(&ModuleManager::stub, this) } },
        { 0x5f7c7a6f, { "cellSailPlayerSetParameter",                       std::bind(&ModuleManager::stub, this) } },
        { 0xbdf21b0f, { "cellSailPlayerBoot",                               std::bind(&ModuleManager::stub, this) } },
        
        { 0x13ea7c64, { "cellGemInit",                                      std::bind(&ModuleManager::stub, this) } },
        
        { 0x7e063bbc, { "cellCameraIsAttached",                             std::bind(&ModuleManager::stub, this) } },
        { 0xbf47c5dd, { "cellCameraInit",                                   std::bind(&ModuleManager::stub, this) } },
        
        { 0x0a563878, { "cellVoiceStart",                                   std::bind(&ModuleManager::stub, this) } },
        { 0x2de54871, { "cellVoiceCreatePort",                              std::bind(&ModuleManager::stub, this) } },
        { 0xae6a21d5, { "cellVoiceConnectIPortToOPort",                     std::bind(&ModuleManager::stub, this) } },
        { 0xc7cf1182, { "cellVoiceInit",                                    std::bind(&ModuleManager::stub, this) } },

        { 0x5c832bd7, { "cellUsbdSetThreadPriority2",                       std::bind(&ModuleManager::stub, this) } },
        { 0xd0e766fe, { "cellUsbdInit",                                     std::bind(&ModuleManager::stub, this) } },

        { 0x1650aea4, { "cellSslEnd",                                       std::bind(&ModuleManager::stub, this) } },
        { 0x571afaca, { "cellSslCertificateLoader",                         std::bind(&CellSsl::cellSslCertificateLoader, &cellSsl) } },
        { 0xfb02c9d2, { "cellSslInit",                                      std::bind(&ModuleManager::stub, this) } },
        
        { 0x250c386c, { "cellHttpInit",                                     std::bind(&ModuleManager::stub, this) } },
        { 0x4e4ee53a, { "cellHttpCreateClient",                             std::bind(&ModuleManager::stub, this) } },
        { 0x9638f766, { "cellHttpInitCookie",                               std::bind(&ModuleManager::stub, this) } },
        
        { 0x522180bc, { "cellHttpsInit",                                    std::bind(&ModuleManager::stub, this) } },
        
        { 0x1395d8d1, { "cellHttpClientSetSslCallback",                     std::bind(&ModuleManager::stub, this) } },
        { 0x434419c8, { "cellHttpClientSetCookieStatus",                    std::bind(&ModuleManager::stub, this) } },
        { 0x473cd9f1, { "cellHttpClientSetRedirectCallback",                std::bind(&ModuleManager::stub, this) } },
        { 0x660d42a9, { "cellHttpClientSetAuthenticationCallback",          std::bind(&ModuleManager::stub, this) } },
        { 0xb6feb84b, { "cellHttpClientSetTransactionStateCallback",        std::bind(&ModuleManager::stub, this) } },
        
        { 0x1e7bff94, { "cellSysCacheMount",                                std::bind(&CellSysCache::cellSysCacheMount, &cellSysCache) } },
        { 0x744c1544, { "cellSysCacheClear",                                std::bind(&CellSysCache::cellSysCacheClear, &cellSysCache) } },
    
        { 0x20543730, { "cellMsgDialogClose",                               std::bind(&ModuleManager::stub, this) } },
        { 0x62b0f803, { "cellMsgDialogAbort",                               std::bind(&ModuleManager::stub, this) } },
        { 0x7603d3db, { "cellMsgDialogOpen2",                               std::bind(&CellMsgDialog::cellMsgDialogOpen2, &cellMsgDialog) } },

        { 0x45fe2fce, { "_sys_spu_printf_initialize",                       std::bind(&ModuleManager::stub, this) } },
        { 0xdd0c1e09, { "_sys_spu_printf_attach_group",                     std::bind(&ModuleManager::stub, this) } },
        { 0xe0da8efd, { "sys_spu_image_close",                              std::bind(&ModuleManager::stub, this), true } },
        { 0xebe5f72f, { "sys_spu_image_import",                             std::bind(&SysPrxForUser::sys_spu_image_import, &sysPrxForUser), true } },

        { 0xe0998dbf, { "sys_prx_get_module_id_by_name",                    std::bind(&ModuleManager::stub, this) } },

        { 0xb48636c4, { "sys_net_show_ifconfig",                            std::bind(&ModuleManager::stub, this) } },
        { 0xfdb8f926, { "sys_net_free_thread_context",                      std::bind(&ModuleManager::stub, this) } },
        
        { 0xe75c40f2, { "sys_process_get_paramsfo",                         std::bind(&ModuleManager::stub, this) } },
        
        { 0x63f63545, { "cellRudpInit",                                     std::bind(&ModuleManager::stub, this) } },
        { 0x6bc587e9, { "cellRudpPollCreate",                               std::bind(&ModuleManager::stub, this) } },
        { 0x6c0cff03, { "cellRudpEnableInternalIOThread",                   std::bind(&ModuleManager::stub, this) } },
        { 0x7ed95e60, { "cellRudpSetEventHandler",                          std::bind(&ModuleManager::stub, this) } },
        { 0xd8310700, { "cellRudpPollWait",                                 std::bind(&ModuleManager::stub, this) } },
        
        { 0x051ee3ee, { "socketpoll",                                       std::bind(&ModuleManager::stub, this) } },
        { 0x13efe7f5, { "getsockname",                                      std::bind(&ModuleManager::stub, this) } },
        { 0x71f4c717, { "gethostbyname",                                    std::bind(&ModuleManager::stub, this) } },
        { 0x858a930b, { "inet_ntoa",                                        std::bind(&ModuleManager::stub, this) } },
        { 0x88f03575, { "setsockopt",                                       std::bind(&ModuleManager::stub, this) } },
        { 0x8af3825e, { "inet_pton",                                        std::bind(&ModuleManager::stub, this) } },
        { 0x9c056962, { "socket",                                           std::bind(&ModuleManager::stub, this) } },
        { 0xb0a59804, { "bind",                                             std::bind(&ModuleManager::stub, this) } },
        { 0xc9157d30, { "_sys_net_h_errno_loc",                             std::bind(&ModuleManager::stub, this) } },
        { 0xdabbc2c0, { "inet_addr",                                        std::bind(&ModuleManager::stub, this) } },

        { 0x05893e7c, { "cellUserTraceRegister",                            std::bind(&ModuleManager::stub, this) } },
        
        { 0x2b761140, { "cellUserInfoGetStat",                              std::bind(&ModuleManager::stub, this) } },
        { 0xc55e338b, { "cellUserInfoGetList",                              std::bind(&ModuleManager::stub, this) } },
        
        { 0x55870804, { "_cellFiberPpuInitialize",                          std::bind(&ModuleManager::stub, this) } },
        { 0x9e25c72d, { "_cellFiberPpuSchedulerAttributeInitialize",        std::bind(&ModuleManager::stub, this) } },
    };
}
