#ifndef AL_FILEWATCHER_H_
#define AL_FILEWATCHER_H_

#include "aldefs.h"
#include "threads.h"

enum FileEvent {
	FILE_INVALID = 0,
	FILE_UNCHANGED = 0x0001,
	FILE_ADDED = 0x0010,
	FILE_REMOVED = 0x0100,
	FILE_MODIFIED = 0x1000
};

typedef void(*PFN_file_event_callback)(const char* filename, void* user_context);
typedef PFN_file_event_callback fw_callback_t;

typedef struct AL_FileEventCallback_ {
	PFN_file_event_callback callback;
	enum FileEvent event;
	void* user_context;
} AL_FileEventCallback;

typedef struct AL_FileWatcher_ {
	AL_Thread thread;
	AL_FileEventCallback* callbacks;
	u64 update_ms;
	void* internals; // implementation defined
} AL_FileWatcher;

ALAPI b8 AL_CreateFileWatcher(const char* path, u64 update_ms, AL_FileWatcher* watcher);

ALAPI void AL_StartFileWatcher(AL_FileWatcher* watcher);

ALAPI b8 AL_DestroyFileWatcher(AL_FileWatcher* watcher);

ALAPI b8 AL_AddFileCallback(AL_FileWatcher* watcher, fw_callback_t callback, enum FileEvent status, void* user_context);

#endif