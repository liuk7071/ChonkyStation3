cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Optimizations
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

# Options
option(ENABLE_USER_BUILD "Enable user build" OFF)
option(ENABLE_QT_BUILD   "Enable Qt6 build"  OFF)

if (ENABLE_USER_BUILD)
    add_compile_definitions(CHONKYSTATION3_USER_BUILD)
endif()

if (ENABLE_QT_BUILD)
    add_compile_definitions(CHONKYSTATION3_QT_BUILD)
endif()

project("ChonkyStation3")

find_package(SDL3 3.2.4 CONFIG)
find_package(toml11 3.7.1 CONFIG)
find_package(xxHash 0.8.3 CONFIG)
add_subdirectory(Dependencies)

add_executable(ChonkyStation3)

target_sources(ChonkyStation3 PRIVATE "ChonkyStation3/ChonkyStation3.cpp" "ChonkyStation3/Loaders/ELF/ELFLoader.hpp" "ChonkyStation3/Loaders/ELF/ELFLoader.cpp" "ChonkyStation3/Common/common.hpp" "ChonkyStation3/PlayStation3.hpp" "ChonkyStation3/PlayStation3.cpp" "ChonkyStation3/Memory/Memory.cpp" "ChonkyStation3/Memory/Memory.hpp" "ChonkyStation3/Common/BEField.hpp" "ChonkyStation3/PPU/PPU.cpp" "ChonkyStation3/PPU/PPU.hpp" "ChonkyStation3/PPU/Backends/PPUInterpreter.hpp" "ChonkyStation3/PPU/Backends/PPUInterpreter.cpp" "Dependencies/Dolphin/BitField.hpp" "ChonkyStation3/PPU/PPUDisassembler.hpp" "ChonkyStation3/PPU/PPUTypes.hpp" "ChonkyStation3/PPU/PPUDisassembler.cpp" "ChonkyStation3/OS/ModuleManager.cpp" "ChonkyStation3/OS/ModuleManager.hpp"  "ChonkyStation3/OS/Syscall.hpp" "ChonkyStation3/OS/Syscall.cpp" "ChonkyStation3/OS/Modules/SysPrxForUser.hpp" "ChonkyStation3/OS/Thread.hpp" "ChonkyStation3/OS/Thread.cpp" "ChonkyStation3/OS/ThreadManager.hpp" "ChonkyStation3/OS/ThreadManager.cpp" "ChonkyStation3/Common/MemoryConstants.hpp" "ChonkyStation3/OS/Modules/SysPrxForUser.cpp" "ChonkyStation3/Common/CellTypes.hpp" "ChonkyStation3/OS/Import.hpp" "ChonkyStation3/OS/Syscalls/sys_memory.cpp" "ChonkyStation3/OS/Syscalls/sys_mmapper.cpp" "ChonkyStation3/OS/Modules/SysThread.hpp" "ChonkyStation3/OS/Modules/SysThread.cpp" "ChonkyStation3/OS/Modules/SysLwMutex.hpp" "ChonkyStation3/OS/Modules/SysLwMutex.cpp" "ChonkyStation3/OS/Modules/SysMMapper.hpp" "ChonkyStation3/OS/Modules/SysMMapper.cpp" "ChonkyStation3/OS/HandleManager.hpp" "ChonkyStation3/Common/ElfSymbolParser.hpp" "ChonkyStation3/OS/Modules/CellGcmSys.hpp" "ChonkyStation3/OS/Modules/CellGcmSys.cpp" "ChonkyStation3/OS/Modules/CellVideoOut.hpp" "ChonkyStation3/OS/Modules/CellVideoOut.cpp" "ChonkyStation3/RSX/RSX.hpp" "ChonkyStation3/RSX/RSX.cpp" "Dependencies/OpenGL/opengl.hpp" "ChonkyStation3/RSX/VertexShaderDecompiler.hpp" "ChonkyStation3/RSX/VertexShaderDecompiler.cpp" "Dependencies/Panda3DS/logger.hpp" "ChonkyStation3/OS/Syscalls/sys_timer.cpp" "ChonkyStation3/Scheduler/Scheduler.cpp" "ChonkyStation3/RSX/FragmentShaderDecompiler.cpp" "ChonkyStation3/OS/Modules/CellSysutil.cpp" "ChonkyStation3/OS/Modules/CellSysmodule.cpp" "ChonkyStation3/OS/Modules/CellResc.cpp" "ChonkyStation3/Loaders/PRX/PRXLoader.cpp" "ChonkyStation3/Loaders/StubPatcher.cpp" "ChonkyStation3/OS/PRXManager.cpp" "ChonkyStation3/OS/Modules/CellGame.cpp" "ChonkyStation3/OS/Modules/CellSpurs.cpp" "ChonkyStation3/OS/Modules/CellRtc.cpp" "ChonkyStation3/OS/Modules/CellFs.cpp" "ChonkyStation3/OS/Syscalls/sys_event_queue.cpp" "ChonkyStation3/Filesystem/Filesystem.cpp" "ChonkyStation3/OS/Modules/CellPngDec.cpp" "Dependencies/lodepng/lodepng.h" "Dependencies/lodepng/lodepng.cpp" "ChonkyStation3/OS/Modules/SceNpTrophy.cpp" "ChonkyStation3/OS/Modules/SceNpTrophy.hpp" "ChonkyStation3/OS/Modules/CellSaveData.cpp" "ChonkyStation3/OS/Modules/CellPad.cpp" "ChonkyStation3/OS/Modules/CellPad.hpp" "ChonkyStation3/Loaders/SFO/SFOLoader.cpp" "ChonkyStation3/Loaders/SFO/SFOLoader.hpp" "ChonkyStation3/Loaders/Game/GameLoader.cpp" "ChonkyStation3/OS/Lv2Object.hpp" "ChonkyStation3/OS/Lv2ObjectManager.hpp" "ChonkyStation3/OS/Syscalls/sys_mutex.cpp" "ChonkyStation3/OS/Lv2Objects/Lv2Mutex.cpp" "ChonkyStation3/OS/Lv2Base.cpp" "ChonkyStation3/OS/Syscalls/sys_cond.cpp" "ChonkyStation3/OS/Syscalls/sys_semaphore.cpp" "ChonkyStation3/OS/Lv2Objects/Lv2Semaphore.cpp" "ChonkyStation3/OS/Modules/CellKb.cpp" "ChonkyStation3/OS/Syscalls/sys_spu.cpp" "ChonkyStation3/OS/Lv2Objects/Lv2LwCond.cpp" "ChonkyStation3/OS/Modules/SysLwCond.cpp" "ChonkyStation3/OS/Modules/CellSsl.cpp" "ChonkyStation3/Frontend/GameWindow.cpp" "ChonkyStation3/OS/Modules/CellSysCache.cpp" "ChonkyStation3/OS/Syscalls/sys_ppu_thread.cpp" "ChonkyStation3/OS/Modules/CellMsgDialog.cpp" "ChonkyStation3/OS/Lv2Objects/Lv2Cond.cpp" "ChonkyStation3/OS/Modules/SceNp.cpp" "ChonkyStation3/OS/Syscalls/sys_prx.cpp" "ChonkyStation3/Loaders/SPU/SPULoader.cpp" "ChonkyStation3/OS/Lv2Objects/Lv2SPUThreadGroup.cpp" "ChonkyStation3/OS/SPUThread.cpp" "ChonkyStation3/OS/SPUThreadManager.cpp" "ChonkyStation3/SPU/SPU.cpp" "ChonkyStation3/SPU/Backends/SPUInterpreter.cpp" "ChonkyStation3/OS/Lv2Objects/Lv2EventQueue.cpp" "ChonkyStation3/OS/Syscalls/sys_vm.cpp" "ChonkyStation3/OS/Syscalls/sys_rwlock.cpp" "ChonkyStation3/OS/Lv2Objects/Lv2RwLock.cpp" "ChonkyStation3/OS/Modules/CellAudio.cpp" "ChonkyStation3/Settings.cpp" "ChonkyStation3/OS/Syscalls/sys_fs.cpp" "ChonkyStation3/OS/Modules/CellAudioOut.cpp" "ChonkyStation3/OS/Syscalls/sys_event_flag.cpp" "ChonkyStation3/OS/Syscalls/sys_event_port.cpp" "ChonkyStation3/RSX/Capture/RSXCaptureReplayer.cpp")
target_include_directories(ChonkyStation3 PUBLIC ChonkyStation3)
target_include_directories(ChonkyStation3 PUBLIC ChonkyStation3/Common)
target_include_directories(ChonkyStation3 PUBLIC ChonkyStation3/Loaders)
target_include_directories(ChonkyStation3 PUBLIC ChonkyStation3/Memory)
target_include_directories(ChonkyStation3 PUBLIC ChonkyStation3/PPU)
target_include_directories(ChonkyStation3 PUBLIC ChonkyStation3/SPU)
target_include_directories(ChonkyStation3 PUBLIC ChonkyStation3/RSX)
target_include_directories(ChonkyStation3 PUBLIC ChonkyStation3/OS)
target_include_directories(ChonkyStation3 PUBLIC ChonkyStation3/Scheduler)
target_include_directories(ChonkyStation3 PUBLIC ChonkyStation3/Filesystem)

target_include_directories(ChonkyStation3 PUBLIC Dependencies/Dolphin)
target_include_directories(ChonkyStation3 PUBLIC Dependencies/ELFIO)
target_include_directories(ChonkyStation3 PUBLIC Dependencies/lodepng)
target_include_directories(ChonkyStation3 PUBLIC Dependencies/OpenGL)
target_include_directories(ChonkyStation3 PUBLIC Dependencies/Panda3DS)

target_link_libraries(ChonkyStation3 PRIVATE glad SDL3::SDL3 toml11::toml11 xxHash::xxhash)

if (ENABLE_QT_BUILD)
    find_package(Qt6 REQUIRED COMPONENTS Widgets)
    target_include_directories(ChonkyStation3 PRIVATE ${Qt6Gui_PRIVATE_INCLUDE_DIRS})
    target_sources(ChonkyStation3 PRIVATE "ChonkyStation3/Frontend/MainWindow.cpp" "ChonkyStation3/Frontend/SettingsWidget.cpp")

    set_target_properties(ChonkyStation3 PROPERTIES AUTOMOC ON)
    set_target_properties(ChonkyStation3 PROPERTIES AUTORCC ON)
    set_target_properties(ChonkyStation3 PROPERTIES AUTOUIC ON)

    qt_standard_project_setup()
    qt_add_ui(ChonkyStation3 SOURCES "ChonkyStation3/Frontend/UI/mainwindow.ui" INCLUDE_PREFIX "Frontend/UI")
    qt_add_ui(ChonkyStation3 SOURCES "ChonkyStation3/Frontend/UI/settings.ui" INCLUDE_PREFIX "Frontend/UI")
    qt_add_resources(ChonkyStation3 "resources" PREFIX "/" FILES Resources/icon-small.ico)

    target_link_libraries(ChonkyStation3 PUBLIC Qt6::Widgets)
endif()

if (WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    target_sources(ChonkyStation3 PRIVATE "Resources/icon.rc")
endif()

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-format-nonliteral -Wno-format-security -Wno-invalid-offsetof")
endif()
