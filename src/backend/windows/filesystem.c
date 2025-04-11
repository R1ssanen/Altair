#include "aldefs.h"
#if defined (AL_PLATFORM_WIN)

#include "filesystem.h"

#include <string.h>
#include <malloc.h>
#include <assert.h>

#include "array.h"
#include "hash.h"
#include "log.h"
#include "threads.h"

b8 AL_CreateDirectory(const char* dirpath, AL_Directory* directory) {
	if (!dirpath) {
		LERROR("Cannot create directory with null path string.");
		return false;
	}

	if (!directory) {
		LERROR("Cannot create directory with null output pointer.");
		return false;
	}

	u64 dirpath_len = strnlen(dirpath, MAX_PATH);

	// fix trailing backslash
	if (dirpath[dirpath_len - 1] != '\\') {
		LWARN("Directory path '%s' doesn't have a trailing backslash; Adding one.", dirpath);
		dirpath_len += 1;

		char* fixed_path = malloc(dirpath_len + 1);
		if (!fixed_path) {
			free(fixed_path);
			LERROR("Could not allocate %lluB of memory for directory name string.", dirpath_len + 1);
			return false;
		}

		strncpy_s(fixed_path, dirpath_len + 1, dirpath, dirpath_len);
		strncat_s(fixed_path, dirpath_len + 1, "\\", 2);
		dirpath = fixed_path;
	}

	directory->dirpath = dirpath;
	directory->files = AL_Array(AL_File, 0);
	directory->hash = FNV_1A(dirpath, strlen(dirpath));

	return true;
}

b8 AL_DestroyDirectory(AL_Directory* directory) {
	if (!directory) {
		LERROR("Cannot destroy null directory.");
		return false;
	}

	AL_ForEach(directory->files, i) {
		AL_File* file = directory->files + i;
		free(file->filepath);
		file->last_write = 0;
		file->status = FILE_INVALID;
	}

	AL_Free(directory->files);
	free(directory->dirpath);

	return true;
}

b8 AL_ListDirectory(AL_Directory* directory, const char* dirpath, const char* filter) {
	if (!directory) {
		LERROR("Cannot list a null directory.");
		return false;
	}

	if (!filter) {
		LWARN("No filter set for list directory; defaulting to wildcard '*'.");
		filter = "*";
	}

	if (!dirpath) dirpath = directory->dirpath;

	assert(directory->files != NULL);
	assert(directory->dirpath != NULL);
	assert(dirpath != NULL);

	u64 dirpath_len = strnlen(dirpath, MAX_PATH);
	assert(dirpath_len > 0);
	assert(dirpath[dirpath_len - 1] == '\\');

	char search_string[MAX_PATH];
	strncpy_s(search_string, MAX_PATH, dirpath, dirpath_len);
	strncat_s(search_string, MAX_PATH, filter, strlen(filter));

	WIN32_FIND_DATAA data;
	HANDLE search_handle = FindFirstFileA(search_string, &data);
	if (search_handle == INVALID_HANDLE_VALUE) {
		if (GetLastError() == ERROR_FILE_NOT_FOUND) {
			LWARN("Directory '%s' is empty.", dirpath);
			return true;
		}

		LERROR("Failed to list directory '%s'.", dirpath);
		return false;
	}

begin_next_file:
	if (strncmp(data.cFileName, ".", 1) == 0) goto next_file;
	if (strncmp(data.cFileName, "..", 2) == 0) goto next_file;

	u64 filename_len = strnlen(data.cFileName, MAX_PATH);
	u64 filepath_len = dirpath_len + filename_len + 1; // +1 for trailing backslash

	char* filepath = malloc(filepath_len + 1);
	if (!filepath) {
		free(filepath);
		LERROR("Could not allocate %lluB of memory for filename string.", filepath_len + 1);
		return false;
	}

	strncpy_s(filepath, filepath_len + 1, dirpath, dirpath_len);
	strncat_s(filepath, filepath_len + 1, data.cFileName, filename_len);

	if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		strncat_s(filepath, filepath_len + 1, "\\", 2);
		if (!AL_ListDirectory(directory, filepath, filter)) {
			LERROR("Could not list subdirectory '%s'.", filepath);
			return false;
		}
	}

	else {
		u64 hash = FNV_1A(filepath, filepath_len);
		u64 last_write = (u64)(data.ftLastWriteTime.dwHighDateTime) << 32 | data.ftLastWriteTime.dwLowDateTime;

		AL_ForEach(directory->files, i) {
			AL_File* file = directory->files + i;

			if (file->hash == hash) {
				if (file->last_write != last_write) file->status = FILE_MODIFIED;
				else								file->status = FILE_UNCHANGED;
				file->last_write = last_write;
				goto next_file;
			}
		}

		AL_File file = (AL_File){ .filepath = filepath, .last_write = last_write, .hash = hash, .status = FILE_ADDED };
		AL_Append(directory->files, file);
	}

next_file:
	if (FindNextFileA(search_handle, &data)) goto begin_next_file;

	FindClose(search_handle);
	return true;
}

void AL_ResetFileStates(AL_Directory* directory) {
	AL_ForEach(directory->files, i) directory->files[i].status = FILE_REMOVED;
}

static u32 __stdcall FilewatchProc(void* argument) {
	AL_FileWatcher* watcher = argument;
	if (!watcher) {
		LERROR("Filewatch process argument is null. Exiting thread process.");
		return 1;
	}

	LNOTE("Starting filewatch process; Watching '%s'.", watcher->directory.dirpath);

	while (1) {
		if (!AL_ListDirectory(&watcher->directory, NULL, watcher->filter))
			LWARN("Could not list directory '%s'.", watcher->directory.dirpath);

		AL_DispatchFileCallbacks(watcher);
		AL_ResetFileStates(&watcher->directory);

		Sleep(watcher->update_ms);
	}
}

b8 AL_CreateFileWatcher(const char* dirpath, u64 update_ms, const char* filter, void* user_context, AL_FileWatcher* watcher) {
	if (!dirpath) {
		LERROR("Cannot create a filewatcher for null directory path.");
		return false;
	}

	AL_Directory directory;
	if (!AL_CreateDirectory(dirpath, &directory)) {
		LERROR("Could not create directory at '%s' for filewatcher.", dirpath);
		return false;
	}

	watcher->directory = directory;
	watcher->user_context = user_context;
	watcher->update_ms = update_ms;
	watcher->filter = filter;

	watcher->on_add = NULL;
	watcher->on_idle = NULL;
	watcher->on_modify = NULL;
	watcher->on_remove = NULL;

	if (!AL_CreateThread((void*)FilewatchProc, (void*)watcher, true, &watcher->thread)) {
		LERROR("Could not create thread for filewatcher of directory '%s'.", dirpath);
		return false;
	}

	LSUCCESS("Filewatch process launched, filtering by '%s'.", filter);
	return true;
}

b8 AL_StartFileWatcher(AL_FileWatcher* watcher) {
	if (!watcher) {
		LERROR("Cannot start a null filewatcher");
		return false;
	}

	return AL_ResumeThread(&watcher->thread);
}

b8 AL_DestroyFileWatcher(AL_FileWatcher* watcher) {
	if (!watcher) {
		LERROR("Cannot destroy null filewatcher");
		return false;
	}

	if (!AL_DestroyThread(&watcher->thread)) {
		LERROR("Could not destroy filewatcher thread 0x%X.", watcher->thread.id);
		return false;
	}

	if (!AL_DestroyDirectory(&watcher->directory)) {
		LERROR("Could not destroy filewatcher directory '%s'.", watcher->directory.dirpath);
		return false;
	}

	watcher->on_add = NULL;
	watcher->on_idle = NULL;
	watcher->on_modify = NULL;
	watcher->on_remove = NULL;

	return true;
}

b8 AL_SetFileCallback(AL_FileWatcher* watcher, enum FileStatus status, fw_callback_t callback) {
	if (!watcher) {
		LERROR("Cannot set file status callbacks for null filewatcher.");
		return false;
	}

	if (!callback)
		LWARN("Setting null file status callback method.");

	switch (status) {
	case FILE_UNCHANGED: watcher->on_idle = callback; break;
	case FILE_ADDED: watcher->on_add = callback; break;
	case FILE_REMOVED: watcher->on_remove = callback; break;
	case FILE_MODIFIED: watcher->on_modify = callback; break;
	default: LERROR("File status 0x%X not supported for callbacks.", status); return false;
	}

	return true;
}

void AL_DispatchFileCallbacks(AL_FileWatcher* watcher) {
	if (!watcher) {
		LERROR("Cannot dispatch file status callbacks of null filewatcher.");
		return;
	}

	assert(watcher->directory.files != NULL);

	AL_ForEach(watcher->directory.files, i) {
		AL_File* file = watcher->directory.files + i;

		switch (file->status) {
		case FILE_UNCHANGED:
			if (watcher->on_idle) watcher->on_idle(file, watcher->user_context);
			break;

		case FILE_ADDED:
			if (watcher->on_add) watcher->on_add(file, watcher->user_context);
			break;

		case FILE_MODIFIED:
			if (watcher->on_modify) watcher->on_modify(file, watcher->user_context);
			break;

		case FILE_REMOVED:
			if (watcher->on_remove) watcher->on_remove(file, watcher->user_context);
			AL_Remove(watcher->directory.files, i--);
			break;

		default: break;
		}
	}
}

#endif
