#include "array.h"
#include "log.h"
#include "manager.h"
#include "plugin.h"
#include "aldefs.h"
#include "filesystem.h"

static void on_add(AL_File* file, void* argument) {
	AL_PluginManager* manager = argument;
	LINFO("Added: %s", file->filepath);
	//if (!RegisterPlugin(manager, file->filepath)) LERROR("Plugin callback failed.");
}

static void on_remove(AL_File* file, void* argument) {
	AL_PluginManager* manager = argument;
	LINFO("Removed: %s", file->filepath);
	//if (!UnregisterPlugin(manager, file->filepath)) LERROR("Plugin callback failed.");
}

static void on_modify(AL_File* file, void* argument) {
	AL_PluginManager* manager = argument;
	LINFO("Modified: %s", file->filepath);
	//if (!UnregisterPlugin(manager, file->filepath)) LERROR("Plugin callback failed.");
}

int main(int argc, char** argv) {

	AL_PluginManager manager;
	if (!AL_CreatePluginManager(&manager)) {
		LERROR("Could not initialize plugin manager.");
		return 1;
	}

	AL_FileWatcher watcher;
	if (!AL_CreateFileWatcher("plugins\\", 1000, "*.dll", (void*)(&manager), &watcher)) {
		LERROR("Could not create filewatcher.");
		return 1;
	}

	AL_SetFileCallback(&watcher, FILE_ADDED, on_add);
	AL_SetFileCallback(&watcher, FILE_REMOVED, on_remove);
	AL_SetFileCallback(&watcher, FILE_MODIFIED, on_modify);
	LINFO("Filewatcher callbacks set. Starting...");

	AL_StartFileWatcher(&watcher);

	for (;;);

	if (!AL_DestroyFileWatcher(&watcher)) {
		LERROR("Could not destroy filewatcher.");
		return 1;
	}

	if (!AL_DestroyPluginManager(&manager)) {
		LERROR("Could not destroy plugin manager.");
		return 1;
	}

	LSUCCESS("Successful shutdown.");
	return 0;
}
