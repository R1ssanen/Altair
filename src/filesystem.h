#ifndef AL_FRONTEND_FILESYSTEM_H_
#define AL_FRONTEND_FILESYSTEM_H_

#include "aldefs.h"
#include "threads.h"

enum FileStatus {
	FILE_INVALID = 0,
	FILE_UNCHANGED,
	FILE_ADDED,
	FILE_REMOVED,
	FILE_MODIFIED
};

typedef struct {
	const char* filepath;
	u64 hash;
	u64 last_write;
	enum FileStatus status;
} AL_File;

typedef struct {
	AL_File* files;
	const char* dirpath;
	u64 hash;
} AL_Directory;

b8 AL_CreateDirectory(const char* dirpath, AL_Directory* directory);

b8 AL_DestroyDirectory(AL_Directory* directory);

b8 AL_ListDirectory(AL_Directory* directory, const char* dirpath, const char* filter);

void AL_ResetFileStates(AL_Directory* directory);

typedef void(*PFN_file_status_callback)(AL_File*, void*);
typedef PFN_file_status_callback fw_callback_t;

typedef struct {
	AL_Directory directory;
	AL_Thread thread;
	void* user_context;
	const char* filter;
	PFN_file_status_callback on_idle;
	PFN_file_status_callback on_add;
	PFN_file_status_callback on_remove;
	PFN_file_status_callback on_modify;
	u64 update_ms;
} AL_FileWatcher;

b8 AL_CreateFileWatcher(const char* dirpath, u64 update_ms, const char* filter, void* user_context, AL_FileWatcher* watcher);

b8 AL_StartFileWatcher(AL_FileWatcher* watcher);

b8 AL_DestroyFileWatcher(AL_FileWatcher* watcher);

b8 AL_SetFileCallback(AL_FileWatcher* watcher, enum FileStatus status, fw_callback_t callback);

void AL_DispatchFileCallbacks(AL_FileWatcher* watcher);

#endif
