#pragma once
#include <cstdarg>
#include <fstream>


namespace Log {
// Our logger class
template <bool enabled>
class Logger {
public:
    Logger(std::string prefix = "") : prefix(prefix) {}

    std::string prefix;

    void log(const char* fmt, ...) {
        if constexpr (!enabled) return;

        std::fputs(prefix.c_str(), stdout);
        std::va_list args;
        va_start(args, fmt);
        std::vprintf(fmt, args);
        va_end(args);
    }

    void logNoPrefix(const char* fmt, ...) {
        if constexpr (!enabled) return;

        std::va_list args;
        va_start(args, fmt);
        std::vprintf(fmt, args);
        va_end(args);
    }
};

// Our loggers here. Enable/disable by toggling the template param

#ifdef CHONKYSTATION3_USER_BUILD
#define true false
#else
//#define true false
#define false true
#endif

// Modules
static Logger cellNetCtl            = Logger<true> ("[Module ][cellNetCtl    ] ");
static Logger cellAudioOut          = Logger<true> ("[Module ][cellAudioOut  ] ");
static Logger cellAudio             = Logger<true> ("[Module ][cellAudio     ] ");
static Logger cellMsgDialog         = Logger<true> ("[Module ][cellMsgDialog ] ");
static Logger cellSysCache          = Logger<true> ("[Module ][cellSysCache  ] ");
static Logger cellSsl               = Logger<true> ("[Module ][cellSsl       ] ");
static Logger cellKb                = Logger<true> ("[Module ][cellKb        ] ");
static Logger cellPad               = Logger<true> ("[Module ][cellPad       ] ");
static Logger cellSaveData          = Logger<true> ("[Module ][cellSaveData  ] ");
static Logger sceNp                 = Logger<true> ("[Module ][sceNp         ] ");
static Logger sceNpTrophy           = Logger<true> ("[Module ][sceNpTrophy   ] ");
static Logger cellPngDec            = Logger<true> ("[Module ][cellPngDec    ] ");
static Logger cellFs                = Logger<true> ("[Module ][cellFs        ] ");
static Logger cellRtc               = Logger<true> ("[Module ][cellRtc       ] ");
static Logger cellSpurs             = Logger<true> ("[Module ][cellSpurs     ] ");
static Logger cellGame              = Logger<true> ("[Module ][cellGame      ] ");
static Logger cellResc              = Logger<false>("[Module ][cellResc      ] ");
static Logger cellSysmodule         = Logger<true> ("[Module ][cellSysmodule ] ");
static Logger cellSysutil           = Logger<true> ("[Module ][cellSysutil   ] ");
static Logger cellGcmSys            = Logger<true> ("[Module ][cellGcmSys    ] ");
static Logger cellVideoOut          = Logger<true> ("[Module ][cellVideoOut  ] ");
static Logger sysLwCond             = Logger<true> ("[Module ][sysLwCond     ] ");
static Logger sysLwMutex            = Logger<true> ("[Module ][sysLwMutex    ] ");
static Logger sysMMapper            = Logger<true> ("[Module ][sysMMapper    ] ");
static Logger sysPrxForUser         = Logger<false>("[Module ][sysPrxForUser ] ");
static Logger sysThread             = Logger<true> ("[Module ][sysThread     ] ");

// Syscalls
static Logger sys_spu               = Logger<true> ("[Syscall][sys_spu       ] ");
static Logger sys_semaphore         = Logger<false>("[Syscall][sys_semaphore ] ");
static Logger sys_ppu_thread        = Logger<true> ("[Syscall][sys_ppu_thread] ");
static Logger sys_event_flag        = Logger<true> ("[Syscall][sys_event_flag] ");
static Logger sys_prx               = Logger<true> ("[Syscall][sys_prx       ] ");
static Logger sys_cond              = Logger<true> ("[Syscall][sys_cond      ] ");
static Logger sys_rwlock            = Logger<true> ("[Syscall][sys_rwlock    ] ");
static Logger sys_mutex             = Logger<false>("[Syscall][sys_mutex     ] ");
static Logger sys_event             = Logger<true> ("[Syscall][sys_event     ] ");
static Logger sys_vm                = Logger<true> ("[Syscall][sys_vm        ] ");
static Logger sys_memory            = Logger<true> ("[Syscall][sys_memory    ] ");
static Logger sys_mmapper           = Logger<true> ("[Syscall][sys_mmapper   ] ");
static Logger sys_timer             = Logger<false>("[Syscall][sys_timer     ] ");
static Logger sys_fs                = Logger<true> ("[Syscall][sys_fs        ] ");
static Logger misc_sc               = Logger<false>("[Syscall][Misc          ] ");
static Logger tty                   = Logger<1>    ();

// RSX
static Logger rsx                   = Logger<true> ("[RSX    ][Command       ] ");
static Logger rsx_capture_replayer  = Logger<true> ("[RSX    ][Capture Replay] ");
static Logger vertex_shader         = Logger<false>("[Shader ][Vertex        ] ");
static Logger fragment_shader       = Logger<false>("[Shader ][Fragment      ] ");
static Logger rsx_cache             = Logger<true> ("[RSX    ][Cache         ] ");

// Loader
static Logger loader_elf            = Logger<1>    ("[Loader ][ELF           ] ");
static Logger loader_self           = Logger<1>    ("[Loader ][SELF To ELF   ] ");
static Logger loader_spu            = Logger<1>    ("[Loader ][SPU           ] ");
static Logger loader_prx            = Logger<1>    ("[Loader ][PRX           ] ");
static Logger loader_sfo            = Logger<0>    ("[Loader ][SFO           ] ");
static Logger loader_pkg            = Logger<1>    ("[Loader ][PKG           ] ");
static Logger loader_game           = Logger<1>    ("[Loader ][Game          ] ");

// Manager
static Logger manager_prx           = Logger<1>    ("[Manager][PRX           ] ");

// Thread
static Logger thread                = Logger<false>("[Thread ][PPU           ] ");
static Logger thread_spu            = Logger<0>("[Thread ][SPU           ] ");

// Other
static Logger memory                = Logger<true> ("[Other  ][Memory        ] ");
static Logger lle_module            = Logger<true> ("[LLE    ][Module        ] ");
static Logger filesystem            = Logger<true> ("[Other  ][Filesystem    ] ");
static Logger lv2_obj               = Logger<true> ("[Other  ][Lv2 Object    ] ");
static Logger unimplemented         = Logger<true> ("[Other  ][Unimplemented ] ");

#undef true
#undef false

// We have 2 ways to create a log function
// MAKE_LOG_FUNCTION: Creates a log function which is toggleable but always killed for user-facing builds
// MAKE_LOG_FUNCTION_USER: Creates a log function which is toggleable, may be on for user builds as well
// We need this because sadly due to the loggers taking variadic arguments, compilers will not properly
// Kill them fully even when they're disabled. The only way they will is if the function with varargs is totally empty

#define MAKE_LOG_FUNCTION_USER(functionName, logger)                    \
	template <typename... Args>                                         \
	void functionName(const char* fmt, Args&&... args) {                \
		Log::logger.log(fmt, args...);                             \
	}                                                                   \
    template <typename... Args>                                         \
    void functionName##NoPrefix(const char* fmt, Args&&... args) {      \
        Log::logger.logNoPrefix(fmt, args...);                     \
    }

#ifdef CHONKYSTATION3_USER_BUILD
#define MAKE_LOG_FUNCTION(functionName, logger)             \
	template <typename... Args>                             \
	void functionName(const char* fmt, Args&&... args) {}   \
    template <typename... Args>                             \
	void functionName##NoPrefix(const char* fmt, Args&&... args) {}
#else
#define MAKE_LOG_FUNCTION(functionName, logger) MAKE_LOG_FUNCTION_USER(functionName, logger)
#endif
}
