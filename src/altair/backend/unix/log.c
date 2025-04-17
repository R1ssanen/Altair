#include "../../aldefs.h"
#if defined(AL_PLATFORM_UNIX)

#    include <assert.h>
#    include <stdarg.h>
#    include <stdio.h>

#    include "../../log.h"

static const char* s_level_colors[] = {
    "\x1b[37m%s\x1b[0m",    // info
    "\x1b[37;4m%s\x1b[0m",  // note
    "\x1b[32m%s\x1b[0m",    // success
    "\x1b[30;43m%s\x1b[0m", // warning
    "\x1b[30;41m%s\x1b[0m"  // error
};

void AL_LogMessage(enum LogLevel level, const char* fmt, ...) {
    assert(
        "Log level must be valid enum" && (level >= LOG_LEVEL_INFO) && (level <= LOG_LEVEL_ERROR)
    );
    assert("Format-string must be valid" && (fmt != NULL));

#    if defined(AL_NO_ESCAPE_CODES)
    char* buf = fmt;
#    else
    char buf[1024];
    snprintf(buf, 1024, s_level_colors[level], fmt);
#    endif

    va_list args;
    va_start(args, fmt);
    vfprintf(level >= LOG_LEVEL_ERROR ? stderr : stdout, buf, args);
    va_end(args);
}

#endif
