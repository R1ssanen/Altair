#ifndef AL_THREADS_H_
#define AL_THREADS_H_

#include "aldefs.h"

typedef struct {
	void* lock;
	void* condition;
	void* handle;
	u32 id;
} AL_Thread;

b8 AL_CreateThread(void* routine, void* argument, b8 create_suspended, AL_Thread* thread);

b8 AL_ResumeThread(AL_Thread* thread);

b8 AL_DestroyThread(AL_Thread* thread);

b8 AL_AwaitThread(AL_Thread* thread, u32 timeout_ms);

b8 AL_AwaitCondition(AL_Thread* thread, u32 timeout_ms);

void AL_WakeCondition(AL_Thread* thread);

void AL_Lock(AL_Thread* thread);

void AL_Unlock(AL_Thread* thread);

#endif
