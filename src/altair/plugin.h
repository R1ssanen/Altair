#ifndef AL_PLUGIN_H_
#define AL_PLUGIN_H_

#include "aldefs.h"
#include "dll.h"
#include "threads.h"

enum PluginType {
    PLUGIN_INVALID  = 0,
    PLUGIN_OTHER    = 0x0001,
    PLUGIN_KEYBOARD = 0x0010,
    PLUGIN_ASYNC    = 0x0100,
};

struct AL_PluginManager_;
struct AL_Plugin_;

typedef b8 (*PFN_plugin_init_t)(struct AL_PluginManager_*, struct AL_Plugin_*);
typedef b8 (*PFN_plugin_cleanup_t)(void);
typedef void (*PFN_plugin_update_t)(u64);

typedef struct AL_Plugin_ {
    union {
        AL_Thread           thread;
        PFN_plugin_update_t update;
    } opt;

    AL_DLL               handle;
    PFN_plugin_cleanup_t cleanup;
    PFN_plugin_init_t    init;
    u64                  uuid;
    enum PluginType      type;
} AL_Plugin;

b8    AL_LoadPlugin(const char* filepath, AL_Plugin* plugin);

b8    AL_UnloadPlugin(AL_Plugin* plugin);

void* AL_Get(AL_Plugin* plugin, const char* symbol, b8 required);

#endif
