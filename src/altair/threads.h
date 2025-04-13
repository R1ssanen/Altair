#ifndef AL_THREADS_H_
#define AL_THREADS_H_

#include "aldefs.h"

#define AL_AWAIT_MAX 0xffffffff

enum SharedFlag {
	SHARED_FLAG_INVALID,
	SHARED_FLAG_EXIT,
	SHARED_FLAG_DEPLOY
};

typedef struct AL_Mutex_ {
	void* internals; // implementation defined
	enum SharedFlag flag;
} AL_Mutex;

ALAPI AL_Mutex AL_CreateMutex(void);

ALAPI void AL_DestroyMutex(AL_Mutex* mutex);

ALAPI void AL_Lock(AL_Mutex* mutex);

ALAPI void AL_Unlock(AL_Mutex* mutex);

ALAPI b8 AL_AwaitCondition(AL_Mutex* mutex, u32 timeout_ms);

ALAPI void AL_WakeCondition(AL_Mutex* mutex);

#define ALSAFE(pmutex, statement) 	\
	do { 							\
		AL_Lock(pmutex); 		 	\
		statement					\
		AL_Unlock(pmutex); 			\
	} while (0)

// resets the flag after reading
ALAPI enum SharedFlag AL_SafeReadFlag(AL_Mutex* mutex);

ALAPI void AL_SafeWriteFlag(AL_Mutex* mutex, enum SharedFlag flag);

typedef struct AL_Thread_ {
	AL_Mutex mutex;
	void* internals; // implementation defined
	void* user_context;
} AL_Thread;

b8 AL_CreateThread(void* routine, void* user_context, b8 launch_immediately, AL_Thread* thread);

void AL_StartThread(AL_Thread* thread);

b8 AL_DestroyThread(AL_Thread* thread, u32 timeout_ms);

#endif
