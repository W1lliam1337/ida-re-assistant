#pragma once

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define IDA_RE_PLATFORM_WINDOWS
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
#elif defined(__APPLE__)
    #define IDA_RE_PLATFORM_MACOS
    #include <TargetConditionals.h>
#elif defined(__linux__)
    #define IDA_RE_PLATFORM_LINUX
#else
    #error "Unsupported platform"
#endif

// Compiler detection
#if defined(_MSC_VER)
    #define IDA_RE_COMPILER_MSVC
    #define IDA_RE_FORCE_INLINE __forceinline
    #define IDA_RE_NO_INLINE    __declspec(noinline)
#elif defined(__clang__)
    #define IDA_RE_COMPILER_CLANG
    #define IDA_RE_FORCE_INLINE inline __attribute__((always_inline))
    #define IDA_RE_NO_INLINE    __attribute__((noinline))
#elif defined(__GNUC__)
    #define IDA_RE_COMPILER_GCC
    #define IDA_RE_FORCE_INLINE inline __attribute__((always_inline))
    #define IDA_RE_NO_INLINE    __attribute__((noinline))
#endif

// Debug/Release detection
#if defined(_DEBUG) || defined(DEBUG) || !defined(NDEBUG)
    #define IDA_RE_DEBUG
#else
    #define IDA_RE_RELEASE
#endif

// Platform-specific includes
#ifdef IDA_RE_PLATFORM_WINDOWS
    #include <Windows.h>
    #include <shlobj.h>
    #pragma comment(lib, "shell32.lib")
#elif defined(IDA_RE_PLATFORM_LINUX) || defined(IDA_RE_PLATFORM_MACOS)
    #include <pwd.h>
    #include <unistd.h>
#endif

// Utility macros
#define IDA_RE_STRINGIFY(x)      #x
#define IDA_RE_STRINGIFY_MACRO(x) IDA_RE_STRINGIFY(x)

#define IDA_RE_CONCAT_IMPL(x, y) x##y
#define IDA_RE_CONCAT(x, y)      IDA_RE_CONCAT_IMPL(x, y)

// Branch prediction hints
#ifdef IDA_RE_COMPILER_MSVC
    #define IDA_RE_LIKELY(x)   (x)
    #define IDA_RE_UNLIKELY(x) (x)
#else
    #define IDA_RE_LIKELY(x)   __builtin_expect(!!(x), 1)
    #define IDA_RE_UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif

// Platform-specific C runtime functions
#ifdef IDA_RE_PLATFORM_WINDOWS
    #define IDA_RE_LOCALTIME(tm_ptr, time_t_ptr) localtime_s(tm_ptr, time_t_ptr)
    #define IDA_RE_GMTIME(tm_ptr, time_t_ptr)    gmtime_s(tm_ptr, time_t_ptr)
    #define IDA_RE_FOPEN(file_ptr, filename, mode) fopen_s(&file_ptr, filename, mode)
    #define IDA_RE_STRDUP(str)                   _strdup(str)
    #define IDA_RE_STRICMP(s1, s2)               _stricmp(s1, s2)
    #define IDA_RE_STRNICMP(s1, s2, n)           _strnicmp(s1, s2, n)
    #define IDA_RE_SNPRINTF                      _snprintf_s
    #define IDA_RE_VSNPRINTF                     _vsnprintf_s
#else
    #define IDA_RE_LOCALTIME(tm_ptr, time_t_ptr) localtime_r(time_t_ptr, tm_ptr)
    #define IDA_RE_GMTIME(tm_ptr, time_t_ptr)    gmtime_r(time_t_ptr, tm_ptr)
    #define IDA_RE_FOPEN(file_ptr, filename, mode) (file_ptr = fopen(filename, mode))
    #define IDA_RE_STRDUP(str)                   strdup(str)
    #define IDA_RE_STRICMP(s1, s2)               strcasecmp(s1, s2)
    #define IDA_RE_STRNICMP(s1, s2, n)           strncasecmp(s1, s2, n)
    #define IDA_RE_SNPRINTF                      snprintf
    #define IDA_RE_VSNPRINTF                     vsnprintf
#endif

// Common C headers
#include <cassert>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

// Common STL headers
#include <algorithm>
#include <atomic>
#include <chrono>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <ranges>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <iomanip>
#include <regex>

// Third-party
#include <nlohmann/json.hpp>

// Global type aliases
using json_t = nlohmann::json;