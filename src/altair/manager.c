#include "manager.h"

#include <assert.h>
#include <string.h>

#include "aldefs.h"
#include "array.h"
#include "hash.h"
#include "log.h"
#include "plugin.h"
#include "threads.h"

b8 AL_CreatePluginManager(AL_PluginManager* manager) {
    LINFO("Initializing plugin manager.");

    if (!manager) {
        LERROR("Cannot initialize null plugin manager.");
        return false;
    }

    manager->mutex    = AL_CreateMutex();
    manager->registry = AL_Array(AL_Plugin, 0);

    LSUCCESS("Plugin manager initialized succesfully.");
    return true;
}

b8 AL_DestroyPluginManager(AL_PluginManager* manager) {
    if (!manager) return true;
    assert(manager->registry != NULL);

    AL_ForEach(manager->registry, i) AL_UnloadPlugin(manager->registry + i);

    AL_Free(manager->registry);
    AL_DestroyMutex(&manager->mutex);

    LSUCCESS("Plugin manager destroyed succesfully.");
    return true;
}

b8 AL_RegisterPlugin(AL_PluginManager* manager, const char* filepath) {
    if (!filepath) {
        LERROR("Cannot register plugin with null filepath.");
        return false;
    }

    if (!manager) {
        LERROR("Cannot register plugin '%s' with null plugin manager.", filepath);
        return false;
    }

    AL_Plugin plugin;
    if (!AL_LoadPlugin(filepath, &plugin)) {
        LERROR("Could not load plugin '%s'.", filepath);
        return false;
    }

    assert(plugin.init != NULL);
    assert(manager->registry != NULL);

    if (!plugin.init(manager, &plugin)) {
        LERROR("Initialization of plugin '%s' failed.", plugin.handle.filepath);
        AL_UnloadPlugin(&plugin);
        return false;
    }

    ALSAFE(&manager->mutex, AL_Append(manager->registry, plugin););
    LSUCCESS("Plugin '%s' succesfully registered.", plugin.handle.filepath);

    if (plugin.type & PLUGIN_ASYNC) AL_StartThread(&plugin.opt.thread);
    return true;
}

b8 AL_UnregisterPlugin(AL_PluginManager* manager, const char* filepath) {
    if (!manager) {
        LERROR("Cannot unregister plugin with null plugin manager.");
        return false;
    }

    if (!filepath) {
        LERROR("Cannot unregister plugin with a null filepath.");
        return false;
    }

    u64 hash = FNV_1A_C(filepath, strlen(filepath));
    assert(manager->registry != NULL);

    AL_ForEach(manager->registry, i) {
        AL_Plugin* plugin = manager->registry + i;

        if (plugin->uuid == hash) {
            ALSAFE(&manager->mutex, {
                AL_UnloadPlugin(plugin);
                AL_Remove(manager->registry, i);
            });
            return true;
        }
    }

    LERROR("Plugin '%s' not found within registry; cannot unregister.", filepath);
    return false;
}

AL_Plugin* AL_Query(AL_PluginManager* manager, const char* filepath, b8 required) {
    if (!manager) {
        LERROR("Cannot query with a null plugin manager.");
        return NULL;
    }

    if (!filepath) {
        LERROR("Cannot query plugin registry with null name.");
        return NULL;
    }

    assert(manager->registry != NULL);
    u64 hash = FNV_1A_C(filepath, strlen(filepath));

    AL_ForEach(manager->registry, i) {
        AL_Plugin* plugin = manager->registry + i;
        if (plugin->uuid == hash) return plugin;
    }

    if (required) LERROR("Plugin '%s' not found from register.", filepath);
    return NULL;
}
