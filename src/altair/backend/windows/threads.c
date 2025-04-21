#include "../../aldefs.h"
#if defined(AL_PLATFORM_WIN)

#    include "../../log.h"
#    include "../../threads.h"

b8 AL_CreateThread(
    PFN_thread_proc_t routine, void* parameter, b8 create_suspended, AL_Thread* thread
) {
    if (!routine) {
        LERROR("Cannot create thread with null routine.");
        return false;
    }

    if (!thread) {
        LERROR("Cannot create thread with null output pointer.");
        return false;
    }

    u32   id     = 0;
    u32   flags  = create_suspended ? CREATE_SUSPENDED : 0;

    void* handle = CreateThread(NULL, 0, routine, parameter, flags, &id);
    if (!handle) {
        LERROR("Could not create thread.");
        return false;
    }

    InitializeConditionVariable(&thread->condition);
    InitializeSRWLock(&thread->lock);

    thread->handle = handle;
    thread->id     = id;

    LSUCCESS("Thread 0x%X launched succesfully.", id);
    return true;
}

b8 AL_DestroyThread(AL_Thread* thread) {
    if (!thread) {
        LERROR("Cannot destroy null thread.");
        return false;
    }

    if (!CloseHandle(thread->handle)) {
        LERROR("Could not close thread handle 0x%X (id=0x%X)", thread->handle, thread->id);
        return false;
    }

    return true;
}

b8 AL_StartThread(AL_Thread* thread) {
    if (ResumeThread((HANDLE)thread->handle) == -1) {
        LERROR("Error resuming thread 0x%X.", thread->id);
        return false;
    }

    return true;
}

b8 AL_AwaitThread(AL_Thread* thread, u32 timeout_ms) {
    if (!thread) {
        LERROR("Cannot await null thread.");
        return false;
    }

    if (WaitForSingleObject(thread->handle, timeout_ms) == WAIT_FAILED) {
        LERROR("Thread 0x%X failed to return to awaiter.", thread->id);
        return false;
    }

    return true;
}

b8 AL_AwaitCondition(AL_Thread* thread, u32 timeout_ms) {
    if (!thread) {
        LERROR("Cannot await null thread condition.");
        return false;
    }

    if (!thread->condition) {
        LERROR("Thread condition must be non-null.");
        return false;
    }

    if (!thread->lock) {
        LERROR("Thread condition must have non-null lock.");
        return false;
    }

    if (!SleepConditionVariableSRW(thread->condition, thread->lock, timeout_ms, 0)) {
        if (GetLastError() == ERROR_TIMEOUT) {
            LERROR("Thread 0x0X timed out of condition await.");
            return false;
        }

        LERROR("Condition await failed.");
        return false;
    }

    return true;
}

void AL_WakeCondition(AL_Thread* thread) {
    WakeConditionVariable((PCONDITION_VARIABLE)thread->condition);
}

void AL_Lock(AL_Thread* thread) { AcquireSRWLockExclusive((PSRWLOCK)thread->lock); }

void AL_Unlock(AL_Thread* thread) { ReleaseSRWLockExclusive((PSRWLOCK)thread->lock); }

#endif
