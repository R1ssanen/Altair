#include "plugin.h"

#include <string.h>
#include <assert.h>

#include "dll.h"
#include "hash.h"
#include "log.h"
#include "aldefs.h"

static u32 sDefaultIdleUpdate(u64 _) { return 1; }

b8 AL_LoadPlugin(const char* filepath, AL_Plugin* plugin) {
	if (!filepath) {
		LERROR("Invalid plugin filepath; loading failed.");
		return false;
	}

	LINFO("Loading plugin '%s'...", filepath);

	if (!plugin) {
		LERROR("Null plugin output pointer; failed to load plugin '%s'.\n", filepath);
		return false;
	}

	AL_DLL handle;
	if (!AL_LoadDLL(filepath, &handle)) {
		LERROR("Can't load plugin '%s'.", filepath);
		return false;
	}

	plugin->init = AL_LoadSymbol(&plugin->handle, "init", true)->addr;
	if (!plugin->init) {
		LERROR("Can't find required 'init' function from plugin '%s'.", filepath);
		return false;
	}

	plugin->update = AL_LoadSymbol(&plugin->handle, "update", false)->addr;
	if (plugin->update) LINFO("Update function found for plugin '%s'.", filepath);
	else plugin->update = sDefaultIdleUpdate;

	plugin->cleanup = AL_LoadSymbol(&plugin->handle, "cleanup", false)->addr;
	if (plugin->cleanup) LINFO("Cleanup function found for plugin '%s'.", filepath);

	plugin->handle = handle;
	plugin->id = FNV_1A(handle.filepath, strlen(handle.filepath));

	LINFO("Plugin '%s' loaded.", filepath);
	return true;
}

b8 AL_UnloadPlugin(AL_Plugin* plugin) {
	if (!plugin) {
		LERROR("Cannot unload null plugin.");
		return false;
	}

	if (plugin->cleanup) {
		if (plugin->cleanup() == PLUGIN_INVALID) {
			LWARN("Internal at-exit cleanup of plugin '%s' failed.", plugin->handle.filepath);
		}
	}

	assert(plugin->handle.filepath != NULL);

	if (!AL_UnloadDLL(&plugin->handle)) {
		LERROR("Plugin '%s' failed to unload.", plugin->handle.filepath);
		return false;
	}

	LSUCCESS("Plugin '%s' (0x%X) succesfully unloaded.", plugin->handle.filepath, plugin->id);
	return true;
}

void* AL_Get(AL_Plugin* plugin, const char* name) {
	if (!plugin) {
		LERROR("Cannot get symbols from null plugin.");
		return NULL;
	}

	if (!name) {
		LERROR("Cannot get symbols from plugin with null name.");
		return NULL;
	}

	assert(plugin->handle.loaded_symbols != NULL);

	void* symbol = AL_FindSymbol(&plugin->handle, name);
	if (!symbol) {
		assert(plugin->handle.filepath != NULL);
		LERROR("Symbol '%s' is not being exported by plugin '%s'.", name, plugin->handle.filepath);
		return NULL;
	}

	return symbol;
}
