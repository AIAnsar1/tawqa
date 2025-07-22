#pragma once

#ifndef TAWQA_GENERIC_HH_INCLUDED
#define TAWQA_GENERIC_HH_INCLUDED

// TAWQA Generic Configuration Header
// Modern C++23 port of netcat's generic.h
// Using TAWQA prefix to avoid naming conflicts

#include <cstdint>
#include <string_view>

// Feature detection macros
#define TAWQA_VOID_MALLOC
#define TAWQA_HAVE_FLOCK
#define TAWQA_HAVE_RANDOM
#define TAWQA_HAVE_LSTAT
#define TAWQA_HAVE_NDBM
#define TAWQA_HAVE_UTMPX
#define TAWQA_HAVE_SETPRIORITY
#define TAWQA_HAVE_SYSINFO

// Standard headers availability
#define TAWQA_HAVE_STDLIB_H
#define TAWQA_HAVE_UNISTD_H
#define TAWQA_HAVE_STDARG_H
#define TAWQA_HAVE_DIRENT_H
#define TAWQA_HAVE_STRINGS_H
#define TAWQA_HAVE_LASTLOG_H
#define TAWQA_HAVE_PATHS_H
#define TAWQA_HAVE_PARAM_H
#define TAWQA_HAVE_SYSMACROS_H
#define TAWQA_HAVE_TTYENT_H

// Security hole feature (disabled by default)
// #define TAWQA_GAPING_SECURITY_HOLE

// Platform-specific adjustments
#ifdef __APPLE__
    // macOS specific settings
    #undef TAWQA_HAVE_UTMPX
    #undef TAWQA_HAVE_SYSINFO
#endif

#ifdef __linux__
    // Linux specific settings
    #undef TAWQA_HAVE_UTMPX
    #undef TAWQA_HAVE_SYSINFO
    #undef TAWQA_HAVE_TTYENT_H
#endif

#ifdef __FreeBSD__
    #undef TAWQA_HAVE_UTMPX
    #undef TAWQA_HAVE_SYSINFO
    #undef TAWQA_HAVE_LASTLOG_H
    #undef TAWQA_HAVE_SYSMACROS_H
#endif

#ifdef __NetBSD__
    #undef TAWQA_HAVE_UTMPX
    #undef TAWQA_HAVE_SYSINFO
    #undef TAWQA_HAVE_LASTLOG_H
#endif

// Modern C++23 type aliases
using tawqa_uint8_t = std::uint8_t;
using tawqa_uint16_t = std::uint16_t;
using tawqa_uint32_t = std::uint32_t;
using tawqa_size_t = std::size_t;

// Constants
constexpr std::size_t TAWQA_BUFFER_SIZE = 8192;
constexpr std::size_t TAWQA_SMALL_BUFFER_SIZE = 256;
constexpr std::uint16_t TAWQA_DEFAULT_PORT = 31337;

// Function declarations
void tawqa_holler(const char* str, const char* p1 = nullptr, 
                  const char* p2 = nullptr, const char* p3 = nullptr);
void tawqa_bail(const char* str, const char* p1 = nullptr, 
                const char* p2 = nullptr, const char* p3 = nullptr);
bool tawqa_doexec(int client_socket);
void tawqa_set_program_path(const char* path);
void tawqa_doexec_cleanup();

#endif // TAWQA_GENERIC_HH_INCLUDED