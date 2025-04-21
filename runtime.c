#include <altair.h>

static void on_add(AL_String directory, AL_String file, void* argument) {
    AL_PluginManager* manager   = argument;

    AL_String         full_path = AL_Copy(directory);
    full_path                   = AL_Concat(full_path, file);

    if (AL_Query(manager, full_path, false)) return;

    AL_RegisterPlugin(manager, full_path);
    AL_Free(full_path);
}

static void on_remove(AL_String directory, AL_String file, void* argument) {
    AL_PluginManager* manager   = argument;

    AL_String         full_path = AL_Copy(directory);
    full_path                   = AL_Concat(full_path, file);

    AL_UnregisterPlugin(manager, full_path);
    AL_Free(full_path);
}

static void on_modify(AL_String dir, AL_String file, void* argument) {
    AL_PluginManager* manager   = argument;

    AL_String         full_path = AL_Copy(dir);
    full_path                   = AL_Concat(full_path, file);

    if (AL_Query(manager, full_path, false)) {
        if (!AL_UnregisterPlugin(manager, full_path)) return AL_Free(full_path);
    }

    if (!AL_RegisterPlugin(manager, full_path)) return;

    AL_Free(full_path);
}

int main(int argc, char* argv[]) {
    AL_String plugins_dir;
    if (argc == 1) {
        const char* parent_dir = AL_PARENT(argv[0]);
        plugins_dir            = AL_CopyC(parent_dir, strlen(parent_dir));
        plugins_dir            = AL_ConcatC(plugins_dir, "/plugins/", 10);
    } else {
        plugins_dir = AL_CopyC(argv[1], strlen(argv[1]));
    }

    AL_PluginManager manager;
    if (!AL_CreatePluginManager(&manager)) {
        LERROR("Could not initialize plugin manager.");
        return 1;
    }

    AL_FileWatcher watcher;
    if (!AL_CreateFileWatcher(plugins_dir, 2, "*.so*", 1000, &watcher)) {
        LERROR("Could not create filewatcher.");
        return 1;
    }

    AL_AddFileCallback(&watcher, on_add, FILE_ADDED, &manager);
    AL_AddFileCallback(&watcher, on_remove, FILE_REMOVED, &manager);
    AL_AddFileCallback(&watcher, on_modify, FILE_MODIFIED, &manager);

    u64 frame = 0;
    AL_AsyncWhile(&manager.mutex, SYNC_EXIT) {
        AL_ForEach(manager.registry, i) {
            AL_Plugin* plugin = manager.registry + i;

            if ((plugin->type & PLUGIN_ASYNC) || !plugin->opt.update) continue;
            else
                plugin->opt.update(frame);
        }
    }

    if (!AL_DestroyFileWatcher(&watcher)) {
        LERROR("Could not destroy filewatcher.");
        return 1;
    }

    if (!AL_DestroyPluginManager(&manager)) {
        LERROR("Could not destroy plugin manager.");
        return 1;
    }

    AL_Free(plugins_dir);

    LSUCCESS("Successful shutdown.");
    return 0;
}
