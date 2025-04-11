#include "manager.h"

#include <assert.h>
#include <string.h>

#include "array.h"
#include "hash.h"
#include "log.h"
#include "plugin.h"
#include "aldefs.h"

b8 AL_CreatePluginManager(AL_PluginManager* manager) {
	LINFO("Initializing plugin manager.");

	if (!manager) {
		LERROR("Cannot initialize null plugin manager.");
		return false;
	}

	manager->registry = AL_Array(AL_Plugin, 1);
	manager->exit = false;

	LSUCCESS("Plugin manager initialized succesfully.");
	return true;
}

b8 AL_DestroyPluginManager(AL_PluginManager* manager) {
	if (!manager) {
		LERROR("Cannot destroy a null plugin manager.");
		return false;
	}

	assert(manager->registry != NULL);

	AL_ForEach(manager->registry, i) AL_UnloadPlugin(manager->registry + i);
	AL_Free(manager->registry);

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

	assert(manager->registry != NULL);

	AL_Plugin plugin;
	if (!AL_LoadPlugin(filepath, &plugin)) {
		LERROR("Could not load plugin '%s'.", filepath);
		return false;
	}

	AL_Append(manager->registry, plugin);
	LSUCCESS("Plugin '%s' succesfully registered.", plugin.handle.filepath);

	if (plugin.init(manager) == PLUGIN_INVALID) {
		LERROR("Initialization of plugin '%s' failed.", plugin.handle.filepath);
		return false;
	}

	LSUCCESS("Plugin '%s' succesfully initialized.", plugin.handle.filepath);
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

	assert(manager->registry != NULL);

	u64 hash = FNV_1A(filepath, strlen(filepath));

	AL_ForEach(manager->registry, i) {
		AL_Plugin* plugin = manager->registry + i;

		if (plugin->id == hash) {
			AL_UnloadPlugin(plugin);
			AL_Remove(manager->registry, i);
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

	u64 plugin_count = AL_Size(manager->registry);
	if (plugin_count == 0) return NULL;

	u64 hash = FNV_1A(filepath, strlen(filepath));

query_again:
	AL_ForEach(manager->registry, i) {
		AL_Plugin* plugin = manager->registry + i;
		if (plugin->id == hash) return plugin;
	}

	if (required) {
		if (!AL_RegisterPlugin(manager, filepath)) {
			LERROR("Could not register required dependency plugin '%s'.", filepath);
			return NULL;
		}
		goto query_again;
	}

	return NULL;
}
