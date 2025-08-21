cmake_minimum_required(VERSION 3.10)

project("ChonkyStation3")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Optimizations
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

add_compile_definitions(SDL_MAIN_HANDLED)

option(ENABLE_USER_BUILD  "Enable user build" OFF)
option(ENABLE_QT_BUILD    "Enable Qt6 build"  OFF)

if (ENABLE_USER_BUILD)
    add_compile_definitions(CHONKYSTATION3_USER_BUILD)
endif()

if (ENABLE_QT_BUILD)
    add_compile_definitions(CHONKYSTATION3_QT_BUILD)
endif()

set(CAPSTONE_ARCHITECTURE_DEFAULT OFF)
set(CAPSTONE_PPC_SUPPORT ON)
set(CAPSTONE_BUILD_MACOS_THIN ON)
add_subdirectory(Dependencies/capstone)
include_directories(Dependencies/capstone/include)

add_subdirectory(Dependencies/OpenGL)
add_subdirectory(Dependencies/SDL)
add_subdirectory(Dependencies/xxHash/cmake_unofficial EXCLUDE_FROM_ALL)
add_subdirectory(Dependencies/toml11)
add_subdirectory(Dependencies/miniaudio)

add_executable(ChonkyStation3)
target_sources(ChonkyStation3 PRIVATE "ChonkyStation3/ChonkyStation3.cpp" "ChonkyStation3/Loaders/ELF/ELFLoader.hpp" "ChonkyStation3/Loaders/ELF/ELFLoader.cpp" "ChonkyStation3/Loaders/ELF/SELFToELF.hpp" "ChonkyStation3/Loaders/ELF/SELFToELF.cpp" "ChonkyStation3/Common/common.hpp" "ChonkyStation3/PlayStation3.hpp" "ChonkyStation3/PlayStation3.cpp" "ChonkyStation3/Memory/Memory.cpp" "ChonkyStation3/Memory/Memory.hpp" "ChonkyStation3/Common/BEField.hpp" "ChonkyStation3/PPU/PPU.cpp" "ChonkyStation3/PPU/PPU.hpp" "ChonkyStation3/PPU/Backends/PPUInterpreter.hpp" "ChonkyStation3/PPU/Backends/PPUInterpreter.cpp" "Dependencies/Dolphin/BitField.hpp" "ChonkyStation3/PPU/PPUDisassembler.hpp" "ChonkyStation3/PPU/PPUTypes.hpp" "ChonkyStation3/PPU/PPUDisassembler.cpp" "ChonkyStation3/OS/ModuleManager.cpp" "ChonkyStation3/OS/ModuleManager.hpp"  "ChonkyStation3/OS/Syscall.hpp" "ChonkyStation3/OS/Syscall.cpp" "ChonkyStation3/OS/Modules/SysPrxForUser.hpp" "ChonkyStation3/OS/Thread.hpp" "ChonkyStation3/OS/Thread.cpp" "ChonkyStation3/OS/ThreadManager.hpp" "ChonkyStation3/OS/ThreadManager.cpp" "ChonkyStation3/Common/MemoryConstants.hpp" "ChonkyStation3/OS/Modules/SysPrxForUser.cpp" "ChonkyStation3/Common/CellTypes.hpp" "ChonkyStation3/OS/Import.hpp" "ChonkyStation3/OS/Syscalls/sys_memory.cpp" "ChonkyStation3/OS/Syscalls/sys_mmapper.cpp" "ChonkyStation3/OS/Modules/SysThread.hpp" "ChonkyStation3/OS/Modules/SysThread.cpp" "ChonkyStation3/OS/Modules/SysLwMutex.hpp" "ChonkyStation3/OS/Modules/SysLwMutex.cpp" "ChonkyStation3/OS/Modules/SysMMapper.hpp" "ChonkyStation3/OS/Modules/SysMMapper.cpp" "ChonkyStation3/OS/HandleManager.hpp" "ChonkyStation3/Common/ElfSymbolParser.hpp" "ChonkyStation3/OS/Modules/CellGcmSys.hpp" "ChonkyStation3/OS/Modules/CellGcmSys.cpp" "ChonkyStation3/OS/Modules/CellVideoOut.hpp" "ChonkyStation3/OS/Modules/CellVideoOut.cpp" "ChonkyStation3/RSX/RSX.hpp" "ChonkyStation3/RSX/RSX.cpp" "Dependencies/OpenGL/opengl.hpp" "ChonkyStation3/RSX/VertexShaderDecompiler.hpp" "ChonkyStation3/RSX/VertexShaderDecompiler.cpp" "Dependencies/Panda3DS/logger.hpp" "ChonkyStation3/OS/Syscalls/sys_timer.cpp" "ChonkyStation3/Scheduler/Scheduler.cpp" "ChonkyStation3/RSX/FragmentShaderDecompiler.cpp" "ChonkyStation3/OS/Modules/CellSysutil.cpp" "ChonkyStation3/OS/Modules/CellSysmodule.cpp" "ChonkyStation3/OS/Modules/CellResc.cpp" "ChonkyStation3/Loaders/PRX/PRXLoader.cpp" "ChonkyStation3/Loaders/StubPatcher.cpp" "ChonkyStation3/OS/PRXManager.cpp" "ChonkyStation3/OS/Modules/CellGame.cpp" "ChonkyStation3/OS/Modules/CellSpurs.cpp" "ChonkyStation3/OS/Modules/CellRtc.cpp" "ChonkyStation3/OS/Modules/CellFs.cpp" "ChonkyStation3/OS/Syscalls/sys_event_queue.cpp" "ChonkyStation3/Filesystem/Filesystem.cpp" "ChonkyStation3/OS/Modules/CellPngDec.cpp" "Dependencies/lodepng/lodepng.h" "Dependencies/lodepng/lodepng.cpp" "ChonkyStation3/OS/Modules/SceNpTrophy.cpp" "ChonkyStation3/OS/Modules/SceNpTrophy.hpp" "ChonkyStation3/OS/Modules/CellSaveData.cpp" "ChonkyStation3/OS/Modules/CellPad.cpp" "ChonkyStation3/OS/Modules/CellPad.hpp" "ChonkyStation3/Loaders/SFO/SFOLoader.cpp" "ChonkyStation3/Loaders/SFO/SFOLoader.hpp" "ChonkyStation3/Loaders/Game/GameLoader.cpp" "ChonkyStation3/Loaders/PKG/PKGInstaller.cpp" "ChonkyStation3/Loaders/PKG/PKGInstaller.hpp" "ChonkyStation3/OS/Lv2Object.hpp" "ChonkyStation3/OS/Lv2ObjectManager.hpp" "ChonkyStation3/OS/Syscalls/sys_mutex.cpp" "ChonkyStation3/OS/Lv2Objects/Lv2Mutex.cpp" "ChonkyStation3/OS/Lv2Base.cpp" "ChonkyStation3/OS/Syscalls/sys_cond.cpp" "ChonkyStation3/OS/Syscalls/sys_semaphore.cpp" "ChonkyStation3/OS/Lv2Objects/Lv2Semaphore.cpp" "ChonkyStation3/OS/Modules/CellKb.cpp" "ChonkyStation3/OS/Syscalls/sys_spu.cpp" "ChonkyStation3/OS/Lv2Objects/Lv2LwCond.cpp" "ChonkyStation3/OS/Modules/SysLwCond.cpp" "ChonkyStation3/OS/Modules/CellSsl.cpp" "ChonkyStation3/Frontend/GameWindow.cpp" "ChonkyStation3/OS/Modules/CellSysCache.cpp" "ChonkyStation3/OS/Syscalls/sys_ppu_thread.cpp" "ChonkyStation3/OS/Modules/CellMsgDialog.cpp" "ChonkyStation3/OS/Lv2Objects/Lv2Cond.cpp" "ChonkyStation3/OS/Modules/SceNp.cpp" "ChonkyStation3/OS/Syscalls/sys_prx.cpp" "ChonkyStation3/Loaders/SPU/SPULoader.cpp" "ChonkyStation3/OS/Lv2Objects/Lv2SPUThreadGroup.cpp" "ChonkyStation3/OS/SPUThread.cpp" "ChonkyStation3/OS/SPUThreadManager.cpp" "ChonkyStation3/SPU/SPU.cpp" "ChonkyStation3/SPU/Backends/SPUInterpreter.cpp" "ChonkyStation3/OS/Lv2Objects/Lv2EventQueue.cpp" "ChonkyStation3/OS/Syscalls/sys_vm.cpp" "ChonkyStation3/OS/Syscalls/sys_rwlock.cpp" "ChonkyStation3/OS/Lv2Objects/Lv2RwLock.cpp" "ChonkyStation3/OS/Modules/CellAudio.cpp" "ChonkyStation3/Settings.cpp" "ChonkyStation3/OS/Syscalls/sys_fs.cpp" "ChonkyStation3/OS/Modules/CellAudioOut.cpp" "ChonkyStation3/OS/Syscalls/sys_event_flag.cpp" "ChonkyStation3/OS/Syscalls/sys_event_port.cpp" "ChonkyStation3/RSX/Capture/RSXCaptureReplayer.cpp" "ChonkyStation3/OS/Lv2Objects/Lv2MemoryContainer.cpp" "ChonkyStation3/OS/Modules/CellNetCtl.cpp" "ChonkyStation3/OS/Lv2Objects/Lv2EventFlag.cpp" "ChonkyStation3/OS/Lv2Objects/Lv2EventFlag.hpp" "ChonkyStation3/Common/Capstone.hpp" "ChonkyStation3/Audio/AudioDevice.hpp" "ChonkyStation3/Audio/miniaudio/MiniaudioDevice.cpp" "ChonkyStation3/Audio/miniaudio/MiniaudioDevice.hpp" "ChonkyStation3/Audio/Null/NullDevice.cpp" "ChonkyStation3/Audio/Null/NullDevice.hpp")
target_sources(ChonkyStation3 PRIVATE "Dependencies/miniaudio/miniaudio.c")
set_target_properties(ChonkyStation3 PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)

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
target_include_directories(ChonkyStation3 PUBLIC ChonkyStation3/Audio)

target_include_directories(ChonkyStation3 PUBLIC Dependencies/ELFIO)
target_include_directories(ChonkyStation3 PUBLIC Dependencies/Dolphin)
target_include_directories(ChonkyStation3 PUBLIC Dependencies/OpenGL)
target_include_directories(ChonkyStation3 PUBLIC Dependencies/Panda3DS)
target_include_directories(ChonkyStation3 PUBLIC ${SDL2_INCLUDE_DIR})
target_include_directories(ChonkyStation3 PUBLIC Dependencies/xxHash)
target_include_directories(ChonkyStation3 PUBLIC Dependencies/lodepng)
target_include_directories(ChonkyStation3 PUBLIC Dependencies/toml11)
target_include_directories(ChonkyStation3 PUBLIC Dependencies/miniaudio)
target_include_directories(ChonkyStation3 PUBLIC Dependencies/plusaes/include)

target_link_libraries(ChonkyStation3 PUBLIC capstone glad SDL2-static xxHash::xxhash toml11::toml11)

if (ENABLE_QT_BUILD)
    add_compile_definitions(QT_NO_OPENGL)
    find_package(Qt6 REQUIRED COMPONENTS Widgets)
    target_include_directories(ChonkyStation3 PRIVATE ${Qt6Gui_PRIVATE_INCLUDE_DIRS})
    target_sources(ChonkyStation3 PRIVATE "ChonkyStation3/Frontend/MainWindow.cpp" "ChonkyStation3/Frontend/MainWindow.hpp" "ChonkyStation3/Frontend/SettingsWidget.cpp" "ChonkyStation3/Frontend/SettingsWidget.hpp" "ChonkyStation3/Frontend/ThreadDebuggerWidget.cpp" "ChonkyStation3/Frontend/ThreadDebuggerWidget.hpp" "ChonkyStation3/Frontend/PPUDebuggerWidget.cpp" "ChonkyStation3/Frontend/PPUDebuggerWidget.hpp" "ChonkyStation3/Frontend/DisabledWidgetOverlay.hpp" "ChonkyStation3/Frontend/MemoryWatchpointDialog.cpp" "ChonkyStation3/Frontend/MemoryWatchpointDialog.hpp" "ChonkyStation3/Frontend/AboutWindow.cpp" "ChonkyStation3/Frontend/AboutWindow.hpp" "ChonkyStation3/Frontend/PKGInstallerOverlay.cpp" "ChonkyStation3/Frontend/PKGInstallerOverlay.hpp")

    set_target_properties(ChonkyStation3 PROPERTIES AUTOMOC ON)
    set_target_properties(ChonkyStation3 PROPERTIES AUTORCC ON)
    set_target_properties(ChonkyStation3 PROPERTIES AUTOUIC ON)

    qt_standard_project_setup()
    qt_add_ui(ChonkyStation3 SOURCES "ChonkyStation3/Frontend/UI/mainwindow.ui" INCLUDE_PREFIX "Frontend/UI")
    qt_add_ui(ChonkyStation3 SOURCES "ChonkyStation3/Frontend/UI/settings.ui" INCLUDE_PREFIX "Frontend/UI")
    qt_add_ui(ChonkyStation3 SOURCES "ChonkyStation3/Frontend/UI/thread_debugger.ui" INCLUDE_PREFIX "Frontend/UI")
    qt_add_ui(ChonkyStation3 SOURCES "ChonkyStation3/Frontend/UI/ppu_debugger.ui" INCLUDE_PREFIX "Frontend/UI")
    qt_add_ui(ChonkyStation3 SOURCES "ChonkyStation3/Frontend/UI/about.ui" INCLUDE_PREFIX "Frontend/UI")
    qt_add_resources(ChonkyStation3 "resources" PREFIX "/" FILES Resources/icon-small.ico)
    qt_add_resources(ChonkyStation3 "resources" PREFIX "/" FILES Resources/Icons/loading.png)

    target_link_libraries(ChonkyStation3 PUBLIC Qt6::Widgets)
endif()

get_target_property(SOURCES ChonkyStation3 SOURCES)
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${SOURCES})

if (WIN32)
    target_sources(ChonkyStation3 PRIVATE "Resources/icon.rc")
endif()

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-format-nonliteral -Wno-format-security -Wno-invalid-offsetof")
endif()

# Detect target architecture
if (NOT APPLE OR "${CMAKE_OSX_ARCHITECTURES}" STREQUAL "")
  # Normal target detection
  if (CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "x86-64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set(HOST_X64 TRUE)
  else()
    set(HOST_X64 FALSE)
  endif()

  if (CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(HOST_ARM64 TRUE)
  else()
    set(HOST_ARM64 FALSE)
  endif()
else()
  # Apple target detection
  if("x86_64" IN_LIST CMAKE_OSX_ARCHITECTURES)
    set(HOST_X64 TRUE)
  else()
    set(HOST_X64 FALSE)
  endif()

  if("arm64" IN_LIST CMAKE_OSX_ARCHITECTURES)
    set(HOST_ARM64 TRUE)
  else()
    set(HOST_ARM64 FALSE)
  endif()

  if (HOST_ARM64 AND HOST_X64)
    message(WARNING "Universal builds will soon not be supported like this! Please compile separately and stitch the builds together!")
  endif()
endif()

if (HOST_X64)
    target_compile_definitions(ChonkyStation3 PRIVATE CHONKYSTATION3_X64_HOST)
    message(STATUS "Detected target architecture: x64")
endif()

if (HOST_ARM64)
    target_compile_definitions(ChonkyStation3 PRIVATE CHONKYSTATION3_ARM64_HOST)
    message(STATUS "Detected target architecture: arm64")
endif()

if (NOT HOST_X64 AND NOT HOST_ARM64)
    message(STATUS "Unknown target architecture")
endif()
