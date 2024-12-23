#pragma once

#include <common.hpp>
#include <unordered_map>
#include <Import.hpp>

#include <Modules/SysPrxForUser.hpp>
#include <Modules/SysThread.hpp>
#include <Modules/SysLwMutex.hpp>
#include <Modules/SysMMapper.hpp>
#include <Modules/CellGcmSys.hpp>
#include <Modules/CellVideoOut.hpp>


// Circular dependency
class PlayStation3;

class ModuleManager {
public:
    ModuleManager(PlayStation3* ps3) : ps3(ps3), sysPrxForUser(ps3), sysThread(ps3), sysLwMutex(ps3), sysMMapper(ps3), cellGcmSys(ps3), cellVideoOut(ps3) {}
    PlayStation3* ps3;

    void call(u32 nid);

    // Map address to import nid
    void registerImport(u32 addr, u32 nid);
    std::unordered_map<u32, u32> imports = {};
    std::unordered_map<u32, Import> import_map {
        { 0x2c847572, { "sysProcessAtExitSpawn",                    std::bind(&SysPrxForUser::sysProcessAtExitSpawn, &sysPrxForUser) }},
        { 0x8461e528, { "sysGetSystemTime",                         std::bind(&SysPrxForUser::sysGetSystemTime, &sysPrxForUser) }},
        { 0x96328741, { "sysProcess_At_ExitSpawn",                  std::bind(&SysPrxForUser::sysProcess_At_ExitSpawn, &sysPrxForUser) }},
        { 0x5267cb35, { "sysSpinlockUnlock",                        std::bind(&SysPrxForUser::sysSpinlockUnlock, &sysPrxForUser) }},
        { 0x8c2bb498, { "sysSpinlockInitialize",                    std::bind(&SysPrxForUser::sysSpinlockInitialize, &sysPrxForUser) }},
        { 0xa285139d, { "sysSpinlockLock",                          std::bind(&SysPrxForUser::sysSpinlockLock, &sysPrxForUser) }},

        { 0x1573dc3f, { "sysLwMutexLock",                           std::bind(&SysLwMutex::sysLwMutexLock, &sysLwMutex) }},
        { 0x1bc200f4, { "sysLwMutexUnlock",                         std::bind(&SysLwMutex::sysLwMutexUnlock, &sysLwMutex) }},
        { 0x2f85c0ef, { "sysLwMutexCreate",                         std::bind(&SysLwMutex::sysLwMutexCreate, &sysLwMutex) }},

        { 0x350d454e, { "sysThreadGetID",                           std::bind(&SysThread::sysThreadGetID, &sysThread) }},
        { 0x744680a2, { "sysThreadInitializeTLS",                   std::bind(&SysThread::sysThreadInitializeTLS, &sysThread) }},

        { 0x409ad939, { "sysMMapperFreeMemory",                     std::bind(&SysMMapper::sysMMapperFreeMemory, &sysMMapper) }},
        { 0x4643ba6e, { "sysMMapperUnmapMemory",                    std::bind(&SysMMapper::sysMMapperUnmapMemory, &sysMMapper) }},
        { 0xb257540b, { "sysMMapperAllocateMemory",                 std::bind(&SysMMapper::sysMMapperAllocateMemory, &sysMMapper) }},

        { 0x15bae46b, { "cellGcmInitBody",                          std::bind(&CellGcmSys::cellGcmInitBody, &cellGcmSys) }},
        { 0xe315a0b2, { "cellGcmGetConfiguration",                  std::bind(&CellGcmSys::cellGcmGetConfiguration, &cellGcmSys) }},

        { 0x0bae8772, { "cellVideoOutConfigure",                    std::bind(&CellVideoOut::cellVideoOutConfigure, &cellVideoOut) }},
        { 0x887572d5, { "cellVideoOutGetState",                     std::bind(&CellVideoOut::cellVideoOutGetState, &cellVideoOut) }},
        { 0xa322db75, { "cellVideoOutGetResolutionAvailability",    std::bind(&CellVideoOut::cellVideoOutGetResolutionAvailability, &cellVideoOut) }},
        { 0xe558748d, { "cellVideoOutGetResolution",                std::bind(&CellVideoOut::cellVideoOutGetResolution, &cellVideoOut) }},
    };

    SysPrxForUser sysPrxForUser;
    SysThread sysThread;
    SysLwMutex sysLwMutex;
    SysMMapper sysMMapper;
    CellGcmSys cellGcmSys;
    CellVideoOut cellVideoOut;

    static Result stub(void* a) { Helpers::panic("Unimplemented function\n"); }
};