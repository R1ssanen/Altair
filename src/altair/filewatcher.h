#ifndef AL_FILEWATCHER_H_
#define AL_FILEWATCHER_H_

#include "aldefs.h"
#include "string.h"
#include "threads.h"

enum FileEvent {
    FILE_INVALID   = 0,
    FILE_DIRECTORY = 0x0001,
    FILE_ADDED     = 0x0010,
    FILE_REMOVED   = 0x0100,
    FILE_MODIFIED  = 0x1000
};

typedef void (*PFN_filewatch_callback_t)(AL_String, AL_String, void* user_context);

typedef struct AL_FileEventCallback_ {
    PFN_filewatch_callback_t callback;
    enum FileEvent           event;
    void*                    user_context;
} AL_FileEventCallback;

typedef struct AL_FileWatcher_ {
    AL_Thread             thread;
    AL_FileEventCallback* callbacks;
    u64                   update_ms;
    void*                 internals; // implementation defined
    const char*           filter;
    u8                    max_depth;
} AL_FileWatcher;

ALAPI b8 AL_CreateFileWatcher(
    const char* path, u8 max_depth, const char* filter, u64 update_ms, AL_FileWatcher* watcher
);

ALAPI b8 AL_DestroyFileWatcher(AL_FileWatcher* watcher);

ALAPI b8 AL_AddFileCallback(
    AL_FileWatcher* watcher, PFN_filewatch_callback_t callback, enum FileEvent event,
    void* user_context
);

#endif
