#include <log.h>
#include <plugin.h>
#include <manager.h>
#include <types.h>

EXPORT u32 init(PluginManager* manager) {
	LINFO("Log test");
	LNOTE("Log test");
	LSUCCESS("Log test");
	LWARN("Log test");
	LERROR("Log test");

	return OTHER;
}

EXPORT u32 update(u64 frame) {
	return 1;
}

EXPORT u32 cleanup(void) {
	LINFO("Plugin 'testing' exiting...");
	return true;
}


