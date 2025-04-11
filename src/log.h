#ifndef AL_LOG_H_
#define AL_LOG_H_

#include "aldefs.h"

enum LogLevel {
	LOG_LEVEL_INFO = 0,
	LOG_LEVEL_NOTE,
	LOG_LEVEL_SUCCESS,
	LOG_LEVEL_WARN,
	LOG_LEVEL_ERROR,
};

ALAPI void AL_LogMessage(enum LogLevel level, const char* fmt, ...);

#if defined (ALCORE)
#define LERROR(fmt, ...)   AL_LogMessage(LOG_LEVEL_ERROR,   "(engine) [ERROR]   " fmt "\n", ##__VA_ARGS__)
#define LWARN(fmt, ...)    AL_LogMessage(LOG_LEVEL_WARN,    "(engine) [WARNING] " fmt "\n", ##__VA_ARGS__)
#define LSUCCESS(fmt, ...) AL_LogMessage(LOG_LEVEL_SUCCESS, "(engine) [SUCCESS] " fmt "\n", ##__VA_ARGS__)
#define LNOTE(fmt, ...)	   AL_LogMessage(LOG_LEVEL_NOTE,    "(engine) [NOTE]    " fmt "\n", ##__VA_ARGS__)
#define LINFO(fmt, ...)    AL_LogMessage(LOG_LEVEL_INFO,    "(engine) [INFO]    " fmt "\n", ##__VA_ARGS__)

#elif defined (ALPLUGIN)
#define LERROR(fmt, ...)   AL_LogMessage(LOG_LEVEL_ERROR,   "(client) [ERROR]   " fmt "\n", ##__VA_ARGS__)
#define LWARN(fmt, ...)    AL_LogMessage(LOG_LEVEL_WARN,    "(client) [WARNING] " fmt "\n", ##__VA_ARGS__)
#define LSUCCESS(fmt, ...) AL_LogMessage(LOG_LEVEL_SUCCESS, "(client) [SUCCESS] " fmt "\n", ##__VA_ARGS__)
#define LNOTE(fmt, ...)	   AL_LogMessage(LOG_LEVEL_NOTE,    "(client) [NOTE]    " fmt "\n", ##__VA_ARGS__)
#define LINFO(fmt, ...)    AL_LogMessage(LOG_LEVEL_INFO,    "(client) [INFO]    " fmt "\n", ##__VA_ARGS__)

#else
#define LERROR(fmt, ...)
#define LWARN(fmt, ...)
#define LSUCCESS(fmt, ...)
#define LNOTE(fmt, ...)
#define LINFO(fmt, ...)

#endif

#endif
