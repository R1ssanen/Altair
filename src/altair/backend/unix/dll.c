#include "../../aldefs.h"
#if defined(AL_PLATFORM_UNIX)

#    include <dlfcn.h>
#    include <malloc.h>

#    include "../../array.h"
#    include "../../dll.h"
#    include "../../hash.h"
#    include "../../log.h"
#    include "../../string.h"

b8 AL_LoadDLL(const char* filepath, AL_DLL* dll) {
    if (!filepath) {
        LERROR("Invalid library filepath; loading failed.");
        return false;
    }

    if (!dll) {
        LERROR("Null DLL output pointer; failed to load DLL '%s'.", filepath);
        return false;
    }

    void* handle = dlopen(filepath, RTLD_LAZY);
    if (!handle) {
        LERROR("Null library handle; cannot load DLL '%s'.\ndlerror: %s", filepath, dlerror());
        return false;
    }

    dll->filepath       = AL_CopyC(filepath, strlen(filepath));
    dll->handle         = handle;
    dll->loaded_symbols = AL_Array(AL_Symbol, 0);

    return true;
}

b8 AL_UnloadDLL(AL_DLL* dll) {
    if (!dll) return true;

    assert(dll->handle != NULL);
    assert(dll->loaded_symbols != NULL);

    if (dlclose(dll->handle) != 0) {
        LERROR("Cannot free DLL '%s'.\ndlerror: %s", dll->filepath, dlerror());
        return false;
    }

    AL_ForEach(dll->loaded_symbols, i) AL_Free(dll->loaded_symbols[i].symname);
    AL_Free(dll->loaded_symbols);

    return true;
}

AL_Symbol* AL_LoadSymbol(AL_DLL* dll, const char* symname, b8 required) {
    if (!symname) {
        LERROR("Cannot load null library symbol.");
        return NULL;
    }

    if (!dll) {
        LERROR("Cannot load symbols from null library.");
        return NULL;
    }

    assert(dll->loaded_symbols != NULL);
    assert(dll->handle != NULL);

    // already loaded
    AL_Symbol* existing = AL_FindSymbol(dll, symname, false);
    if (existing) return existing;

    void* addr = dlsym(dll->handle, symname);
    if (!addr) {
        if (required) LERROR("Symbol '%s' not found within library '%s'.", symname, dll->filepath);
        return NULL;
    }

    AL_Symbol symbol = { .addr = addr, .symname = AL_CopyC(symname, strlen(symname)) };
    AL_Append(dll->loaded_symbols, symbol);

    return AL_Last(dll->loaded_symbols);
}

AL_Symbol* AL_FindSymbol(AL_DLL* dll, const char* symname, b8 required) {
    if (!dll) {
        LERROR("Cannot find symbol from null DLL.");
        return NULL;
    }

    if (!symname) {
        LERROR("Cannot search DLL '%s' with a null symbol name string.", dll->filepath);
        return NULL;
    }

    u64 hash = FNV_1A_C(symname, strlen(symname));
    assert(dll->loaded_symbols != NULL);

    AL_ForEach(dll->loaded_symbols, i) {
        AL_Symbol* symbol = dll->loaded_symbols + i;

        assert(symbol->symname != NULL);
        if (*AL_Metadata(symbol->symname) == hash) return symbol;
    }

    if (required) LERROR("Symbol '%s' not found within DLL '%s'.", symname, dll->filepath);
    return NULL;
}

#endif
