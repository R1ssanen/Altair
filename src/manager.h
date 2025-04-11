#ifndef AL_MANAGER_H_
#define AL_MANAGER_H_

#include "aldefs.h"
#include "plugin.h"

typedef ALAPI struct {
	AL_Plugin* registry;
	b8 exit;
} AL_PluginManager;

b8 AL_CreatePluginManager(AL_PluginManager* manager);

b8 AL_DestroyPluginManager(AL_PluginManager* manager);

ALAPI b8 AL_RegisterPlugin(AL_PluginManager* manager, const char* filepath);

ALAPI b8 AL_UnregisterPlugin(AL_PluginManager* manager, const char* filepath);

ALAPI AL_Plugin* AL_Query(AL_PluginManager* manager, const char* name, b8 required);

#endif
