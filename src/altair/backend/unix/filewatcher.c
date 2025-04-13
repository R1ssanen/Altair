#include "../../aldefs.h"
#if defined (AL_PLATFORM_UNIX)

#include <malloc.h>
#include <sys/inotify.h>
#include <errno.h>

#include "../../filewatcher.h"
#include "../../log.h"
#include "../../array.h"
#include "../../threads.h"

typedef struct inotify_event file_event_t;

typedef struct {
    u32 instance;
    u32 watch_descriptor;
} UnixFileWatcherInternal;

static void s_DispatchFileEventCallbacks(AL_FileWatcher*, const file_event_t*);

static u32 s_FileWatcherProc(void* argument) {
    if (!argument) {
        LERROR("Filewatcher process launched with null internal argument.");
        return 1;
    }

    AL_FileWatcher* watcher = argument;
    UnixFileWatcherInternal* internals = watcher->internals;
    
    assert(watcher->internals != NULL);
    assert(internals->instance != -1);
    assert(internals->watch_descriptor != -1);

    u64 buffer_size = sizeof(file_event_t) + AL_MAX_PATH + 2;
    u8 buffer[buffer_size];

    while (AL_SafeReadFlag(&watcher->thread.mutex) != SHARED_FLAG_EXIT) {
        LINFO("Polling");

        if (read(internals->instance, (void*)buffer, buffer_size) == -1) {
            switch (errno) {
                case EAGAIN: break;
            }
        } else {
            const file_event_t* event = (const file_event_t*)buffer;
            s_DispatchFileEventCallbacks(watcher, event);
        }

        usleep(watcher->update_ms * 1E3);
    }

    LSUCCESS("Filewatcher succesfully exiting.");
    
    AL_WakeCondition(&watcher->thread.mutex);
    return 0;
}

b8 AL_CreateFileWatcher(const char* path, u64 update_ms, AL_FileWatcher* watcher) {
    if (!path) {
        LERROR("Cannot create file watcher for a null path.");
        return false;
    }

    if (!watcher) {
        LERROR("Cannot create a null file watcher.");
        return false;
    }

    u32 instance = inotify_init1(IN_NONBLOCK);
    if (instance == -1) {
        LERROR("Could not create new inotify instance for filewatcher.");
        return false;
    }

    u32 watch_descriptor = inotify_add_watch(instance, path, 
        IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | IN_CLOSE_WRITE);
    if (watch_descriptor == -1) {
        LERROR("Could not add an inotify watch for filewatcher.");
        return false;
    }

    watcher->internals = malloc(sizeof(UnixFileWatcherInternal));
    UnixFileWatcherInternal* internals = watcher->internals;
    internals->instance = instance;
    internals->watch_descriptor = watch_descriptor;

    watcher->update_ms = update_ms;
    watcher->callbacks = AL_Array(AL_FileEventCallback, 3);

    if (!AL_CreateThread((void*)s_FileWatcherProc, watcher, false, &watcher->thread)) {
        LERROR("Could not create new thread process for filewatcher of path '%s'.", path);
        return false;
    }

    return true;
}

void AL_StartFileWatcher(AL_FileWatcher* watcher) {
    if (!watcher) {
        LERROR("Cannot start a null filewatcher.");
        return;
    }

    AL_StartThread(&watcher->thread);
}

b8 AL_DestroyFileWatcher(AL_FileWatcher* watcher) {
    if (!watcher) {
        LERROR("Cannot destroy null filewatcher.");
        return false;
    }

    if (!AL_DestroyThread(&watcher->thread, AL_AWAIT_MAX)) {
        LERROR("Could not destroy filewatcher thread process.");
        return false;
    }

    assert(watcher->callbacks != NULL);

    AL_ForEach(watcher->callbacks, i) {
        AL_FileEventCallback* callback = watcher->callbacks + i;
        callback->callback = NULL;
        callback->user_context = NULL;
        callback->event = 0;
    }

    free(watcher->internals);
    watcher->update_ms = 0;

    return true;
}

b8 AL_AddFileCallback(AL_FileWatcher* watcher, fw_callback_t callback, enum FileEvent status, void* user_context) {
    if (!watcher) {
        LERROR("Cannot add file event callback for null filewatcher.");
        return false;
    }

    if (!callback) {
        LERROR("Cannot add null file event callback for filewatcher.");
        return false;
    }

    assert(watcher->callbacks != NULL);

    AL_ForEach(watcher->callbacks, i) {
        AL_FileEventCallback* fwcb = watcher->callbacks + i;
        if (fwcb->callback == callback) {
            LWARN("Callback 0x%X already exists within filewatcher specified.");
            return true;
        }
    }

    AL_FileEventCallback fwcb = { .callback = callback, .event = status, .user_context = user_context };
    ALSAFE(&watcher->thread.mutex,
        AL_Append(watcher->callbacks, fwcb);
    );
    
    return true;
}

b8 AL_RemoveFileCallback(AL_FileWatcher* watcher, fw_callback_t callback) {
    if (!watcher) {
        LERROR("Cannot add file event callback for null filewatcher.");
        return false;
    }

    if (!callback) {
        LERROR("Cannot add null file event callback for filewatcher.");
        return false;
    }

    assert(watcher->callbacks != NULL);

    AL_ForEach(watcher->callbacks, i) {
        AL_FileEventCallback* fwcb = watcher->callbacks + i;
        if (fwcb->callback == callback) {
            ALSAFE(&watcher->thread.mutex,
                AL_Remove(watcher->callbacks, i);
            );
            return true;
        }
    }
    
    LERROR("Callback 0x%X is not within the filewatcher specified; cannot remove.");
    return false;
}

static enum FileEvent s_TranslateFileEventType(u32 mask) {
    enum FileEvent type = FILE_INVALID;
    
    if (mask & IN_CREATE)      type |= FILE_ADDED;
    if (mask & IN_MOVED_TO)    type |= FILE_ADDED;
    if (mask & IN_DELETE)      type |= FILE_REMOVED;
    if (mask & IN_MOVED_FROM)  type |= FILE_REMOVED;
    if (mask & IN_CLOSE_WRITE) type |= FILE_MODIFIED;

    return type;
}

void s_DispatchFileEventCallbacks(AL_FileWatcher* watcher, const file_event_t* event) {
    enum FileEvent type = s_TranslateFileEventType(event->mask);

    AL_ForEach(watcher->callbacks, i) {
        AL_FileEventCallback* fwcb = watcher->callbacks + i;
        if (type & fwcb->event)
        fwcb->callback(event->name, fwcb->user_context);
    } 
}

#endif