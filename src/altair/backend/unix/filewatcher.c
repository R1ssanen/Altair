#include "../../aldefs.h"
#if defined(AL_PLATFORM_UNIX)

#    include <errno.h>
#    include <fnmatch.h>
#    include <malloc.h>
#    include <stdio.h>
#    include <stdlib.h>
#    include <sys/inotify.h>

#    include "../../array.h"
#    include "../../filewatcher.h"
#    include "../../log.h"
#    include "../../threads.h"

static enum FileEvent s_TranslateFileEventType(u32 mask) {
    enum FileEvent type = FILE_INVALID;

    if (mask & IN_CREATE) type |= FILE_ADDED;
    if (mask & IN_MOVED_TO) type |= FILE_ADDED;
    if (mask & IN_DELETE) type |= FILE_REMOVED;
    if (mask & IN_MOVED_FROM) type |= FILE_REMOVED;
    if (mask & IN_CLOSE_WRITE) type |= FILE_MODIFIED;
    if (mask & IN_ISDIR) type |= FILE_DIRECTORY;

    return type;
}

typedef struct {
    AL_String directory;
    u32       desc;
    u8        depth;
} WatchDirectory;

typedef struct {
    WatchDirectory* watches;
    u32             instance;
    u32             mask;
} UnixFileWatcherInternal;

static u32  s_FileWatcherProc(void* argument);
static void s_BuiltInDirectoryCallback(AL_String directory, AL_String file, void* argument);

b8          AL_CreateFileWatcher(
             const char* path, u8 max_depth, const char* filter, u64 update_ms, AL_FileWatcher* watcher
         ) {
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

    u32 mask = IN_ATTRIB | IN_MASK_CREATE | IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO |
               IN_CLOSE_WRITE;

    u32 watch_descriptor = inotify_add_watch(instance, path, mask);
    if (watch_descriptor == -1) {
        LERROR("Could not add an inotify watch for filewatcher.");
        return false;
    }

    watcher->internals                 = malloc(sizeof(UnixFileWatcherInternal));
    UnixFileWatcherInternal* internals = watcher->internals;
    internals->instance                = instance;
    internals->watches                 = AL_Array(WatchDirectory, 1);
    internals->mask                    = mask;

    WatchDirectory watch               = { .directory = AL_CopyC(path, strlen(path)),
                                           .depth     = 1,
                                           .desc      = watch_descriptor };
    AL_Append(internals->watches, watch);

    watcher->update_ms = update_ms;
    watcher->callbacks = AL_Array(AL_FileEventCallback, 3);
    watcher->filter    = filter ? filter : "*";
    watcher->max_depth = max_depth;

    if (!AL_CreateThread((void*)s_FileWatcherProc, watcher, false, &watcher->thread)) {
        LERROR("Could not create new thread process for filewatcher of path '%s'.", path);
        return false;
    }

    AL_AddFileCallback(watcher, s_BuiltInDirectoryCallback, FILE_DIRECTORY, watcher);

    char command[AL_MAX_PATH];
    sprintf(command, "touch %s* >/dev/null 2>&1", path);
    system(command);

    AL_StartThread(&watcher->thread);
    return true;
}

b8 AL_DestroyFileWatcher(AL_FileWatcher* watcher) {
    if (!watcher) return true;

    assert(watcher->callbacks != NULL);
    assert(watcher->internals != NULL);

    if (!AL_DestroyThread(&watcher->thread, AL_AWAIT_MAX)) {
        LERROR("Could not destroy filewatcher thread process.");
        return false;
    }

    UnixFileWatcherInternal* internals = watcher->internals;

    AL_ForEach(internals->watches, i) {
        WatchDirectory* watch = internals->watches + i;
        inotify_rm_watch(internals->instance, watch->desc);
        AL_Free(watch->directory);
    }

    AL_Free(internals->watches);
    free(watcher->internals);
    AL_Free(watcher->callbacks);

    return true;
}

b8 AL_AddFileCallback(
    AL_FileWatcher* watcher, PFN_filewatch_callback_t callback, enum FileEvent event,
    void* user_context
) {
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

    AL_FileEventCallback fwcb = { .callback     = callback,
                                  .event        = event,
                                  .user_context = user_context };
    ALSAFE(&watcher->thread.mutex, AL_Append(watcher->callbacks, fwcb););

    return true;
}

b8 AL_RemoveFileCallback(AL_FileWatcher* watcher, PFN_filewatch_callback_t callback) {
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
            ALSAFE(&watcher->thread.mutex, AL_Remove(watcher->callbacks, i););
            return true;
        }
    }

    LERROR("Callback 0x%X is not within the filewatcher specified; cannot remove.");
    return false;
}

// static methods

void s_BuiltInDirectoryCallback(AL_String directory, AL_String file, void* argument) {
    AL_FileWatcher*          watcher   = argument;
    UnixFileWatcherInternal* internals = watcher->internals;

    AL_String                full_path = AL_Copy(directory);
    full_path                          = AL_Concat(full_path, file);
    AL_ConcatC(full_path, "/\0", 2);

    u8 depth = 0xff;
    AL_ForEach(internals->watches, i) {
        WatchDirectory* watch = internals->watches + i;
        if (AL_Equals(watch->directory, directory)) depth = watch->depth + 1;
        if (AL_Equals(watch->directory, full_path)) return AL_Free(full_path);
    }

    if (depth > watcher->max_depth) return AL_Free(full_path);

    u32 watch_descriptor = inotify_add_watch(internals->instance, full_path, internals->mask);
    if (watch_descriptor == -1) {
        switch (errno) {
        case EEXIST: LWARN("Subdirectory '%s' is already watched.", full_path);
        default: break;
        }
    } else {
        WatchDirectory watch = { .depth = depth, .directory = full_path, .desc = watch_descriptor };
        AL_Append(internals->watches, watch);
        LINFO("Subdirectory '%s' added to filewatch.", full_path);
    }

    char command[AL_MAX_PATH];
    sprintf(command, "touch %s/* >/dev/null 2>&1", full_path);
    system(command);
}

u32 s_FileWatcherProc(void* argument) {
    if (!argument) {
        LERROR("Filewatcher process launched with null internal argument.");
        return 1;
    }

    AL_FileWatcher*          watcher   = argument;
    UnixFileWatcherInternal* internals = watcher->internals;

    assert(watcher->internals != NULL);
    assert(internals->watches != NULL);
    assert(internals->instance != -1);

    u64 base_size = sizeof(struct inotify_event);
    u64 max_size  = base_size + AL_MAX_PATH + 2;
    u8  buffer[max_size];

    while (AL_SafeReadFlag(&watcher->thread.mutex) != SHARED_FLAG_EXIT) {

        ssize_t bytes_read = read(internals->instance, (void*)buffer, max_size);
        if (bytes_read == -1) {
            switch (errno) {
            case EAGAIN: break;
            default: break;
            }

            usleep(watcher->update_ms * 1E3);
            continue;
        }

        for (u64 byte = 0; byte < bytes_read;) {
            struct inotify_event* event = (struct inotify_event*)(buffer + byte);
            byte += base_size + event->len;
            if (event->len == 0) continue;

            // if filename matches filter
            if (!watcher->filter || (event->mask & IN_ISDIR))
                ; // doesnt match
            else if (fnmatch(watcher->filter, event->name, 0) != 0)
                continue;

            AL_String directory;
            AL_ForEach(internals->watches, i) {
                WatchDirectory* watch = internals->watches + i;
                if (event->wd == watch->desc) {
                    directory = watch->directory;
                    break;
                }
            }

            enum FileEvent mask = s_TranslateFileEventType(event->mask);

            AL_ForEach(watcher->callbacks, i) {
                AL_FileEventCallback* fwcb = watcher->callbacks + i;
                if (mask & fwcb->event) {
                    fwcb->callback(
                        directory, AL_CopyC(event->name, event->len), fwcb->user_context
                    );
                }
            }
        }
    }

    LSUCCESS("Filewatcher succesfully exiting.");

    // signal main thread
    AL_WakeCondition(&watcher->thread.mutex);
    return 0;
}

#endif
