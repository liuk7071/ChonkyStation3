#pragma once

#include <cstdio>
#include <cstdint>
#include <cstdarg>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <iterator>
#include <cstring>

// Types
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using s8 = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;

using uptr = std::uintptr_t;

constexpr size_t operator""_KB(unsigned long long int x) { return 1024ULL * x; }
constexpr size_t operator""_MB(unsigned long long int x) { return 1024_KB * x; }
constexpr size_t operator""_GB(unsigned long long int x) { return 1024_MB * x; }

namespace fs = std::filesystem;

static constexpr u64 CPU_FREQ = 3200000000;

namespace Helpers {
template <class... Args>
[[noreturn]] static void panic(const char* fmt, Args&&... args) {
    std::string error;
    error.resize(512_KB);
    std::sprintf(error.data(), fmt, args...);
    throw std::runtime_error(error);
}

template <class... Args>
static void debugAssert(bool cond, const char* fmt, Args&&... args) {
    if (!cond) [[unlikely]] {
        std::string error;
        error.resize(512_KB);
        std::sprintf(error.data(), fmt, args...);
        throw std::runtime_error(error);
    }
}

static auto readBinary(const fs::path& directory) -> std::vector<u8> {
    std::ifstream file(directory, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "Couldn't find ROM at " << directory << "\n";
        exit(1);
    }

    std::vector<uint8_t> binary;
    file.unsetf(std::ios::skipws);
    std::streampos fileSize;
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    binary.insert(binary.begin(),
        std::istream_iterator<uint8_t>(file),
        std::istream_iterator<uint8_t>());
    file.close();

    return binary;
}

static void dump(const char* filename, u8* data, u64 size) {
    std::ofstream file(filename, std::ios::binary);
    file.write((const char*)data, size);
}

template<typename T>
static inline bool inRange(T num, T start, T end) {
    if ((start <= num) && (num <= end)) return true;
    else return false;
}

template<typename T>
static inline bool inRangeSized(T num, T start, T size) {
    if ((start <= num) && (num < (start + size))) return true;
    else return false;
}

static std::string readString(u8* ptr) {
    std::string str;
    while (*ptr)
        str += *ptr++;
    return str;
}

static std::string readString(const u8* ptr) {
    std::string str;
    while (*ptr)
        str += *ptr++;
    return str;
}

static inline u16 bswap16(u16 val) {
    return (val >> 8) | (val << 8);
}

static inline u32 bswap32(u32 val) {
    u32 byte0 = val         & 0xff;
    u32 byte1 = (val >> 8)  & 0xff;
    u32 byte2 = (val >> 16) & 0xff;
    u32 byte3 = (val >> 24) & 0xff;
    return (byte0 << 24) | (byte1 << 16) | (byte2 << 8) | byte3;
}

static inline u64 bswap64(u64 val) {
    u64 byte0 = val         & 0xff;
    u64 byte1 = (val >> 8)  & 0xff;
    u64 byte2 = (val >> 16) & 0xff;
    u64 byte3 = (val >> 24) & 0xff;
    u64 byte4 = (val >> 32) & 0xff;
    u64 byte5 = (val >> 40) & 0xff;
    u64 byte6 = (val >> 48) & 0xff;
    u64 byte7 = (val >> 56) & 0xff;
    return (byte0 << 56) | (byte1 << 48) | (byte2 << 40) | (byte3 << 32) | (byte4 << 24) | (byte5 << 16) | (byte6 << 8) | byte7;
}

template<typename T>
static inline T bswap(T val) {
    if constexpr (sizeof(T) == sizeof(u8)) return val;
    else if constexpr (sizeof(T) == sizeof(u16)) {
#ifdef _MSC_VER
        return _byteswap_ushort(val);
#else
        return (val >> 8) | (val << 8);
#endif
    }
    else if constexpr (sizeof(T) == sizeof(u32)) {
#ifdef _MSC_VER
        return _byteswap_ulong(val);
#else
        u32 byte0 = val & 0xff;
        u32 byte1 = (val >> 8) & 0xff;
        u32 byte2 = (val >> 16) & 0xff;
        u32 byte3 = (val >> 24) & 0xff;
        return (byte0 << 24) | (byte1 << 16) | (byte2 << 8) | byte3;
#endif
    }
    else if constexpr (sizeof(T) == sizeof(u64)) {
#ifdef _MSC_VER
        return _byteswap_uint64(val);
#else
        u64 byte0 = val & 0xff;
        u64 byte1 = (val >> 8) & 0xff;
        u64 byte2 = (val >> 16) & 0xff;
        u64 byte3 = (val >> 24) & 0xff;
        u64 byte4 = (val >> 32) & 0xff;
        u64 byte5 = (val >> 40) & 0xff;
        u64 byte6 = (val >> 48) & 0xff;
        u64 byte7 = (val >> 56) & 0xff;
        return (byte0 << 56) | (byte1 << 48) | (byte2 << 40) | (byte3 << 32) | (byte4 << 24) | (byte5 << 16) | (byte6 << 8) | byte7;
#endif
    }
    else return 0;
}
}   // End namespace Helpers