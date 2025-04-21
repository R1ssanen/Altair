#ifndef AL_THREADS_H_
#define AL_THREADS_H_

#include "aldefs.h"

#define AL_TIMEOUT_MAX 0xffffffff

enum SyncFlag {
    SYNC_UNSET,
    SYNC_EXIT,
    SYNC_WAIT,
    SYNC_START,
};

typedef struct AL_Mutex_ {
    void*         internals; // implementation defined
    enum SyncFlag flag;
} AL_Mutex;

ALAPI AL_Mutex AL_CreateMutex(void);

ALAPI void     AL_DestroyMutex(AL_Mutex* mutex);

ALAPI void     AL_Lock(AL_Mutex* mutex);

ALAPI void     AL_Unlock(AL_Mutex* mutex);

ALAPI b8       AL_AwaitCondition(AL_Mutex* mutex, u32 timeout_ms);

ALAPI void     AL_WakeCondition(AL_Mutex* mutex);

#define ALSAFE(pmutex, statement)                                                                  \
    do {                                                                                           \
        AL_Lock(pmutex);                                                                           \
        statement AL_Unlock(pmutex);                                                               \
    } while (0)

ALAPI void AL_WriteSyncFlag(AL_Mutex* mutex, enum SyncFlag flag);

// resets the flag after reading
ALAPI enum SyncFlag AL_ReadSyncFlag(AL_Mutex* mutex);

#define AL_AsyncWhile(pmutex, flag) while (AL_ReadSyncFlag(pmutex) != (flag))

typedef struct AL_Thread_ {
    AL_Mutex mutex;
    void*    internals; // implementation defined
    void*    user_context;
} AL_Thread;

typedef u32 (*PFN_thread_proc_t)(void*);

b8 AL_CreateThread(
    PFN_thread_proc_t routine, void* user_context, b8 launch_immediately, AL_Thread* thread
);

b8        AL_DestroyThread(AL_Thread* thread, u32 timeout_ms);

void      AL_StartThread(AL_Thread* thread);

ALAPI u64 AL_GetPid(const AL_Thread* thread);

#endif
