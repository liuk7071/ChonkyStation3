#pragma once

#include <common.hpp>
#include <BEField.hpp>

#include <unordered_set>


#define ARG0 ps3->ppu->state.gprs[3]
#define ARG1 ps3->ppu->state.gprs[4]
#define ARG2 ps3->ppu->state.gprs[5]
#define ARG3 ps3->ppu->state.gprs[6]
#define ARG4 ps3->ppu->state.gprs[7]
#define ARG5 ps3->ppu->state.gprs[8]
#define ARG6 ps3->ppu->state.gprs[9]
#define ARG7 ps3->ppu->state.gprs[10]
#define ARG8 ps3->mem.read<u64>(ps3->ppu->state.gprs[1] + 0x70)
#define ARG9 ps3->mem.read<u64>(ps3->ppu->state.gprs[1] + 0x74)

// Use and modify these macros to do stuff whenever an arg is accessed via the lambdas
//#define ARG0 ([&]() -> u64 { u64 val = ps3->ppu->state.gprs[3]; return val; })(); ps3->ppu->state.gprs[3]
//#define ARG1 ([&]() -> u64 { u64 val = ps3->ppu->state.gprs[4]; return val; })(); ps3->ppu->state.gprs[4]
//#define ARG2 ([&]() -> u64 { u64 val = ps3->ppu->state.gprs[5]; return val; })(); ps3->ppu->state.gprs[5]
//#define ARG3 ([&]() -> u64 { u64 val = ps3->ppu->state.gprs[6]; return val; })(); ps3->ppu->state.gprs[6]
//#define ARG4 ([&]() -> u64 { u64 val = ps3->ppu->state.gprs[7]; return val; })(); ps3->ppu->state.gprs[7]
//#define ARG5 ([&]() -> u64 { u64 val = ps3->ppu->state.gprs[8]; return val; })(); ps3->ppu->state.gprs[8]

namespace CellTypes {

enum Result : u32 {
    CELL_OK         = 0x00000000,
    CELL_CANCEL     = 0x00000001,
    CELL_EAGAIN     = 0x80010001,
    CELL_EINVAL     = 0x80010002,
    CELL_ENOSYS     = 0x80010003,
    CELL_ENOMEM     = 0x80010004,
    CELL_ESRCH      = 0x80010005,
    CELL_ENOENT     = 0x80010006,
    CELL_EDEADLK    = 0x80010008,
    CELL_EPERM      = 0x80010009,
    CELL_EBUSY      = 0x8001000A,
    CELL_ETIMEDOUT  = 0x8001000B,
    CELL_EFAULT     = 0x8001000D,
    CELL_EALIGN     = 0x80010010,
    CELL_EISCONN    = 0x80010015,
    CELL_ENOTCONN   = 0x80010016,
    CELL_EEXIST     = 0x80010014,
    CELL_BADF       = 0x8001002A,
    CELL_EIO        = 0x8001002B,
};

struct CellFsStat {
    BEField<u32> mode;
    BEField<s32> uid;
    BEField<s32> gid;
    BEField<s64> atime;
    BEField<s64> mtime;
    BEField<s64> ctime;
    BEField<u64> size;
    BEField<u64> blksize;
};

static const std::unordered_set<std::string> module_list = {
    "sys_net",
    "cellHttp",
    "cellHttpUtil",
    "cellSsl",
    "cellHttps",
    "libvdec",
    "cellAdec",
    "cellDmux",
    "cellVpost",
    "cellRtc",
    "cellSpurs",
    "cellOvis",
    "cellSheap",
    "cellSync",
    "sys_fs",
    "cellJpgDec",
    "cellGcmSys",
    "cellAudio",
    "cellPamf",
    "cellAtrac",
    "cellNetCtl",
    "cellSysutil",
    "cellSysmodule",
    "sceNp",
    "sys_io",
    "cellPngDec",
    "cellFont",
    "cellFontFT",
    "cell_FreeType2",
    "cellUsbd",
    "cellSail",
    "cellL10n",
    "cellResc",
    "cellDaisy",
    "cellKey2char",
    "cellMic",
    "cellCamera",
    "cellVdecMpeg2",
    "cellVdecAvc",
    "cellAdecLpcm",
    "cellAdecAc3",
    "cellAdecAtx",
    "cellAdecAt3",
    "cellDmuxPamf",
    "sys_lv2dbg",
    "cellSysutilAvcExt",
    "cellUsbPspcm",
    "cellSysutilAvconfExt",
    "cellUserInfo",
    "cellSaveData",
    "cellSubDisplay",
    "cellRec",
    "cellVideoExportUtility",
    "cellGameExec",
    "sceNp2",
    "cellSysutilAp",
    "sceNpClans",
    "cellOskExtUtility",
    "cellVdecDivx",
    "cellJpgEnc",
    "cellGame",
    "cellBGDLUtility",
    "cell_FreeType2",
    "cellVideoUpload",
    "cellSysconfExtUtility",
    "cellFiber",
    "sceNpCommerce2",
    "sceNpTus",
    "cellVoice",
    "cellAdecCelp8",
    "cellCelp8Enc",
    "cellSysutilMisc",
    "cellMusicUtility",
    "libad_core",
    "libad_async",
    "libad_billboard_util",
    "cellScreenShotUtility",
    "cellMusicDecodeUtility",
    "cellSpursJq",
    "cellPngEnc",
    "cellMusicDecodeUtility",
    "libmedi",
    "cellSync2",
    "sceNpUtil",
    "cellRudp",
    "sceNpSns",
    "libgem",
    "cellCrossController",
    "cellCelpEnc",
    "cellGifDec",
    "cellAdecCelp",
    "cellAdecM2bc",
    "cellAdecM4aac",
    "cellAdecMp3",
    "cellImeJpUtility",
    "cellMusicUtility",
    "cellPhotoUtility",
    "cellPrintUtility",
    "cellPhotoImportUtil",
    "cellMusicExportUtility",
    "cellPhotoDecodeUtil",
    "cellSearchUtility",
    "cellSysutilAvc2",
    "cellSailRec",
    "sceNpTrophy",
    "cellSysutilNpEula",
    "cellAdecAt3multi",
    "cellAtracMulti"
};


}   // End namespce OSTypes
