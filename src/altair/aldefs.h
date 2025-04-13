#ifndef AL_DEFS_H_
#define AL_DEFS_H_

#if (defined(_WIN32) || defined(_WIN64))

#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>

#    define AL_PLATFORM_WIN
#    define AL_MAX_PATH (MAX_PATH - 1)

#    if defined(ALCORE) || defined(ALPLUGIN)
#        define ALAPI __declspec(dllexport)

#    else
#        define ALAPI __declspec(dllimport)

#    endif

#elif (defined(__linux__) || defined(__unix__) || defined(__unix))

#    include <unistd.h>
#    include <linux/limits.h>

#    define AL_PLATFORM_UNIX
#    define AL_MAX_PATH (PATH_MAX - 1)

#    if (defined(__GNUC__) || defined(__clang__))
#        define ALAPI __attribute__((visibility("default")))

#    else
#        pragma error "Linux C compiler not supported."

#    endif

#else
#    pragma error "OS not supported."
#endif

#define true  1
#define false 0

typedef _Bool              b8;

typedef unsigned char      u8;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef float              f32;
typedef double             f64;

#endif
