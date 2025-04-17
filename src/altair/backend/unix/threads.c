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

    return (AL_Mutex){ .internals = internals, .flag = SHARED_FLAG_INVALID };
}

void AL_DestroyMutex(AL_Mutex* mutex) {
    if (!mutex) return;

    assert(mutex->internals);
    UnixMutexInternal* internals = mutex->internals;

    pthread_mutex_destroy(&internals->lock);
    pthread_cond_destroy(&internals->cond);

    free(mutex->internals);
    mutex->flag = SHARED_FLAG_INVALID;
}

void AL_Lock(AL_Mutex* mutex) {
    if (!mutex) {
        LERROR("Cannot lock a null mutex.");
        return;
    }

    UnixMutexInternal* internals = mutex->internals;
    pthread_mutex_lock(&internals->lock);
}

void AL_Unlock(AL_Mutex* mutex) {
    if (!mutex) {
        LERROR("Cannot unlock a null mutex.");
        return;
    }

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
    if (timeout_ms == AL_AWAIT_MAX) {
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

enum SharedFlag AL_SafeReadFlag(AL_Mutex* mutex) {
    if (!mutex) {
        LERROR("Cannot safely read shared flag from null mutex.");
        return SHARED_FLAG_INVALID;
    }

    enum SharedFlag flag = SHARED_FLAG_INVALID;

    ALSAFE(mutex, {
        flag        = mutex->flag;
        mutex->flag = SHARED_FLAG_INVALID;
    });

    return flag;
}

void AL_SafeWriteFlag(AL_Mutex* mutex, enum SharedFlag flag) {
    if (!mutex) {
        LERROR("Cannot safelu write shared flag to null mutex.");
        return;
    }

    ALSAFE(mutex, mutex->flag = flag;);
}

void AL_WakeCondition(AL_Mutex* mutex) {
    if (!mutex) {
        LERROR("Cannon wake a condition on null mutex.");
        return;
    }

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

    ALSAFE(&thread->mutex, {
        while ((thread->mutex.flag != SHARED_FLAG_DEPLOY) &&
               (thread->mutex.flag != SHARED_FLAG_EXIT)) {
            AL_AwaitCondition(&thread->mutex, AL_AWAIT_MAX);
        }
        thread->mutex.flag = SHARED_FLAG_INVALID;
    });

    LINFO("Thread 0x%X deployed; entering process routine.", internals->pid);
    assert(internals->routine != NULL);
    internals->routine(thread->user_context);

    return NULL;
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
        LERROR("Could not create new thread.");
        return false;
    }

    pthread_detach(internals->pid);

    if (launch_immediately) {
        LSUCCESS("Thread 0x%X created and launched succesfully.", internals->pid);
        AL_SafeWriteFlag(&thread->mutex, SHARED_FLAG_DEPLOY);
        AL_WakeCondition(&thread->mutex);
        return true;

    } else {
        LSUCCESS("Thread 0x%X created succesfully.", internals->pid);
        return true;
    }
}

// if thread was created with 'launch_immediately=false'
void AL_StartThread(AL_Thread* thread) {
    if (!thread) {
        LERROR("Cannot start a null thread.");
        return;
    }

    assert(thread->internals != NULL);
    UnixThreadInternal* internals = thread->internals;

    AL_SafeWriteFlag(&thread->mutex, SHARED_FLAG_DEPLOY);
    AL_WakeCondition(&thread->mutex);
}

b8 AL_DestroyThread(AL_Thread* thread, u32 timeout_ms) {
    if (!thread) return true;

    AL_SafeWriteFlag(&thread->mutex, SHARED_FLAG_EXIT);

    assert(thread->internals != NULL);
    UnixThreadInternal* internals = thread->internals;
    LINFO(
        "Thread 0x%X requested to exit; awaiting with timeout of %lums...", internals->pid,
        timeout_ms
    );

    if (AL_AwaitCondition(&thread->mutex, timeout_ms))
        LINFO("Thread 0x%X succesfully returned.", internals->pid);

    AL_DestroyMutex(&thread->mutex);
    free(thread->internals);

    return true;
}

#endif
