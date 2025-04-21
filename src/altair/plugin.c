#include "plugin.h"

#include <assert.h>
#include <string.h>

#include "aldefs.h"
#include "array.h"
#include "dll.h"
#include "hash.h"
#include "log.h"

static u32 s_DefaultIdleUpdate(u64 _) { return 0; }

b8         AL_LoadPlugin(const char* filepath, AL_Plugin* plugin) {
    if (!filepath) {
        LERROR("Invalid plugin filepath; loading failed.");
        return false;
    }

    if (!plugin) {
        LERROR("Null plugin output pointer; failed to load plugin '%s'.\n", filepath);
        return false;
    }

    if (!AL_LoadDLL(filepath, &plugin->handle)) {
        LERROR("Can't load plugin '%s'.", filepath);
        return false;
    }

    plugin->uuid    = *AL_Metadata(plugin->handle.filepath);

    AL_Symbol* type = AL_LoadSymbol(&plugin->handle, "type", true);
    if (!type) {
        LERROR("Can't find required 'type' enum from plugin '%s'.", filepath);
        return false;
    } else {
        plugin->type = *(enum PluginType*)type->addr;
    }

    if (plugin->type & PLUGIN_ASYNC) {
        AL_Symbol* proc = AL_LoadSymbol(&plugin->handle, "proc", true);
        if (!proc) {
            LERROR("Can't find required 'proc' function for asynchronous plugin '%s'.", filepath);
            return false;
        }

        if (!AL_CreateThread((PFN_thread_proc_t)proc->addr, plugin, false, &plugin->opt.thread)) {
            LERROR("Could not create thread process for asynchronous plugin '%s'.", filepath);
            return false;
        }
    } else {
        AL_Symbol* update = AL_LoadSymbol(&plugin->handle, "update", false);
        if (update) plugin->opt.update = update->addr;
        else
            plugin->opt.update = NULL;
    }

    AL_Symbol* init = AL_LoadSymbol(&plugin->handle, "init", false);
    if (init) plugin->init = init->addr;
    else
        plugin->init = NULL;

    AL_Symbol* cleanup = AL_LoadSymbol(&plugin->handle, "cleanup", false);
    if (cleanup) plugin->cleanup = cleanup->addr;
    else
        plugin->cleanup = NULL;

    LINFO("Plugin '%s' loaded.", filepath);
    return true;
}

b8 AL_UnloadPlugin(AL_Plugin* plugin) {
    if (!plugin) {
        LERROR("Cannot unload null plugin.");
        return false;
    }

    assert(plugin->handle.filepath != NULL);

    if (plugin->type & PLUGIN_ASYNC) {
        if (!AL_DestroyThread(&plugin->opt.thread, AL_TIMEOUT_MAX)) {
            LERROR(
                "Could not destroy thread of asynchronous plugin '%s'.", plugin->handle.filepath
            );
            return false;
        }
    }

    if (plugin->cleanup) {
        if (!plugin->cleanup())
            LWARN("Internal at-exit cleanup of plugin '%s' failed.", plugin->handle.filepath);
    }

    if (!AL_UnloadDLL(&plugin->handle)) {
        LERROR("Plugin '%s' failed to unload.", plugin->handle.filepath);
        return false;
    }

    LSUCCESS("Plugin '%s' (0x%X) succesfully unloaded.", plugin->handle.filepath, plugin->uuid);
    return true;
}

void* AL_Get(AL_Plugin* plugin, const char* name, b8 required) {
    if (!plugin) {
        LERROR("Cannot get symbols from null plugin.");
        return NULL;
    }

    if (!name) {
        LERROR("Cannot get symbols from plugin with null name.");
        return NULL;
    }

    assert(plugin->handle.loaded_symbols != NULL);
    assert(plugin->handle.filepath != NULL);

    AL_Symbol* symbol = AL_LoadSymbol(&plugin->handle, name, required);
    if (symbol) return symbol->addr;

    if (required)
        LERROR("Symbol '%s' is not being exported by plugin '%s'.", name, plugin->handle.filepath);
    return NULL;
}
