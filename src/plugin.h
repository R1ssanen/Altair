#ifndef AL_PLUGIN_H_
#define AL_PLUGIN_H_

#include "aldefs.h"
#include "dll.h"
#include "aldefs.h"

enum PluginType {
	PLUGIN_INVALID = 0,
	PLUGIN_OTHER,
	PLUGIN_KEYBOARD,
};

typedef struct {
	AL_DLL handle;
	u32(*update)(u64);
	u32(*init)(struct PluginManager*); // required
	u32(*cleanup)(void);
	enum PluginType type;
	u64 id;
} AL_Plugin;

b8 AL_LoadPlugin(const char* filepath, AL_Plugin* plugin);

b8 AL_UnloadPlugin(AL_Plugin* plugin);

void* AL_Get(AL_Plugin* plugin, const char* symbol);

#endif
