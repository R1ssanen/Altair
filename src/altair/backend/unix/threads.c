#include "../../aldefs.h"
#if defined(AL_PLATFORM_UNIX)

#    include <assert.h>
#    include <errno.h>
#    include <malloc.h>
#    include <pthread.h>
#    include <string.h>
#    include <time.h>

#    include "../../log.h"
#    include "../../threads.h"

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t  cond;
} UnixMutexInternal;

typedef struct {
    pthread_t         pid;
    PFN_thread_proc_t routine;
} UnixThreadInternal;

AL_Mutex AL_CreateMutex(void) {
    void*              internals = malloc(sizeof(UnixMutexInternal));
    UnixMutexInternal* mutex     = internals;

    mutex->lock                  = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    mutex->cond                  = (pthread_cond_t)PTHREAD_COND_INITIALIZER;

    return (AL_Mutex){ .internals = internals, .flag = SYNC_UNSET };
}

void AL_DestroyMutex(AL_Mutex* mutex) {
    if (!mutex) return;

    assert(mutex->internals);
    UnixMutexInternal* internals = mutex->internals;

    pthread_mutex_destroy(&internals->lock);
    pthread_cond_destroy(&internals->cond);
    free(mutex->internals);
}

void AL_Lock(AL_Mutex* mutex) {
    if (!mutex) return LERROR("Cannot lock a null mutex.");
    UnixMutexInternal* internals = mutex->internals;
    pthread_mutex_lock(&internals->lock);
}

void AL_Unlock(AL_Mutex* mutex) {
    if (!mutex) return LERROR("Cannot unlock a null mutex.");
    UnixMutexInternal* internals = mutex->internals;
    pthread_mutex_unlock(&internals->lock);
}

b8 AL_AwaitCondition(AL_Mutex* mutex, u32 timeout_ms) {
    if (!mutex) {
        LERROR("Cannot await a condition on null mutex.");
        return false;
    }

    assert(mutex->internals != NULL);

    UnixMutexInternal* internals = mutex->internals;
    if (timeout_ms == AL_TIMEOUT_MAX) {
        pthread_cond_wait(&internals->cond, &internals->lock);
    }

    else {
        u64             seconds     = timeout_ms / 1000;
        u64             nanoseconds = (timeout_ms % 1000) * 1E6;
        struct timespec timeout     = { .tv_sec = seconds, .tv_nsec = nanoseconds };

        if (pthread_cond_timedwait(&internals->cond, &internals->lock, &timeout) == ETIMEDOUT) {
            LWARN("Condition await timed out.");
            return false;
        }
    }

    return true;
}

enum SyncFlag AL_ReadSyncFlag(AL_Mutex* mutex) {
    if (!mutex) {
        LERROR("Cannot safely read sync flag from null mutex.");
        return SYNC_UNSET;
    }

    enum SyncFlag flag;

    ALSAFE(mutex, {
        flag        = mutex->flag;
        mutex->flag = SYNC_UNSET;
    });

    return flag;
}

void AL_WriteSyncFlag(AL_Mutex* mutex, enum SyncFlag flag) {
    if (!mutex) return LERROR("Cannot safely write sync flag to null mutex.");
    ALSAFE(mutex, mutex->flag = flag;);
}

void AL_WakeCondition(AL_Mutex* mutex) {
    if (!mutex) return LERROR("Cannon wake a condition on null mutex.");
    UnixMutexInternal* internals = mutex->internals;
    pthread_cond_signal(&internals->cond);
}

static void* s_ThreadProcWrapper(void* argument) {
    if (!argument) {
        LERROR("Null argument passed to thread process wrapper; exiting thread process.");
        return NULL;
    }

    AL_Thread* thread = argument;
    assert(thread->internals != NULL);
    UnixThreadInternal* internals = thread->internals;
    enum SyncFlag       flag;

    ALSAFE(&thread->mutex, {
        while (thread->mutex.flag == SYNC_UNSET) AL_AwaitCondition(&thread->mutex, AL_TIMEOUT_MAX);
        flag               = thread->mutex.flag;
        thread->mutex.flag = SYNC_UNSET;
    });

    switch (flag) {
    case SYNC_START:
        LSUCCESS("Thread 0x%X started succesfully.", internals->pid);
        assert(internals->routine != NULL);
        internals->routine(thread->user_context);
        break;

    case SYNC_EXIT: LNOTE("Thread 0x%X destroyed prematurely.", internals->pid); break;

    default: LERROR("Unknown sync flag enum."); break;
    }

    AL_WakeCondition(&thread->mutex);
    pthread_exit(NULL);
}

b8 AL_CreateThread(
    PFN_thread_proc_t routine, void* user_context, b8 launch_immediately, AL_Thread* thread
) {
    if (!routine) {
        LERROR("Cannot create thread with null routine.");
        return false;
    }

    if (!thread) {
        LERROR("Cannot create thread with null output pointer.");
        return false;
    }

    thread->mutex                 = AL_CreateMutex();
    thread->user_context          = user_context;

    thread->internals             = malloc(sizeof(UnixThreadInternal));
    UnixThreadInternal* internals = thread->internals;
    internals->routine            = routine;

    if (pthread_create(&internals->pid, NULL, s_ThreadProcWrapper, thread) != 0) {
        LERROR("Could not create new thread process.");
        return false;
    }

    pthread_detach(internals->pid);

    if (launch_immediately) {
        AL_WriteSyncFlag(&thread->mutex, SYNC_START);
        AL_WakeCondition(&thread->mutex);
    }

    return true;
}

// if thread was created with 'launch_immediately=false'
void AL_StartThread(AL_Thread* thread) {
    if (!thread) {
        LERROR("Cannot start a null thread.");
        return;
    }

    assert(thread->internals != NULL);
    UnixThreadInternal* internals = thread->internals;

    AL_WriteSyncFlag(&thread->mutex, SYNC_START);
    AL_WakeCondition(&thread->mutex);
}

b8 AL_DestroyThread(AL_Thread* thread, u32 timeout_ms) {
    if (!thread) return true;
    assert(thread->internals != NULL);

    UnixThreadInternal* internals = thread->internals;
    pthread_t           pid       = internals->pid;

    LINFO("Syncing 0x%X", pid);
    AL_WriteSyncFlag(&thread->mutex, SYNC_EXIT);
    AL_WakeCondition(&thread->mutex);

    ALSAFE(&thread->mutex, {
        /*while (thread->mutex.flag != SYNC_WAIT) {
            if (!AL_AwaitCondition(&thread->mutex, timeout_ms)) {
                LERROR("Thread 0x%X timed out; could not destroy.", pid);
                return false;
            }
        }*/
        if (!AL_AwaitCondition(&thread->mutex, timeout_ms)) {
            LERROR("Thread 0x%X timed out; could not destroy.", pid);
            return false;
        } else {
            LNOTE("Await complete");
        }
    });

    AL_DestroyMutex(&thread->mutex);
    free(thread->internals);

    LSUCCESS("Thread 0x%X succesfully destroyed.", pid);
    return true;
}

u64 AL_GetPid(const AL_Thread* thread) {
    UnixThreadInternal* internals = thread->internals;
    return internals->pid;
}

#endif
