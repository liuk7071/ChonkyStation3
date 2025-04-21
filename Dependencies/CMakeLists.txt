set(BUILD_SHARED_LIBS OFF)
set(BUILD_TESTING OFF)
set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL ON SYSTEM ON)

# Set CMP0069 policy to building external targets with LTO enabled
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)

# OpenGL
add_subdirectory(OpenGL)

# SDL3
if (NOT TARGET SDL3::SDL3)
    set(SDL_SHARED OFF)
    set(SDL_STATIC ON)
    set(SDL_DISKAUDIO OFF)
    set(SDL_TEST_LIBRARY OFF)
    set(SDL_PIPEWIRE OFF)
    add_subdirectory(SDL3)
endif()

# Toml11
if (NOT TARGET toml11::toml11)
    add_subdirectory(toml11)
endif()

# xxHash
if (NOT TARGET xxHash::xxhash)
    add_subdirectory(xxHash/cmake_unofficial)
endif()
