#pragma once

#include <common.hpp>
#include <logger.hpp>
#include <CellTypes.hpp>
#include <BEField.hpp>


// Circular dependency
class PlayStation3;

using namespace CellTypes;

class SysPrxForUser {
public:
    SysPrxForUser(PlayStation3* ps3) : ps3(ps3) {}
    PlayStation3* ps3;
    MAKE_LOG_FUNCTION(log, sysPrxForUser);

    u64 sysProcessExit();
    u64 sysProcessAtExitSpawn();
    u64 sysGetSystemTime();
    u64 sysProcess_At_ExitSpawn();
    u64 sysSpinlockInitialize();
    u64 sysSpinlockLock();
    u64 sysSpinlockUnlock();
};