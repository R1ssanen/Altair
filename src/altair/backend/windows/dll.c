#include "../../aldefs.h"
#if defined(AL_PLATFORM_WIN)

#    include <assert.h>
#    include <malloc.h>
#    include <string.h>

#    include "../../array.h"
#    include "../../dll.h"
#    include "../../hash.h"
#    include "../../log.h"

b8 AL_LoadDLL(const char* filepath, AL_DLL* dll) {
    if (!filepath) {
        LERROR("Invalid library filepath; loading failed.");
        return false;
    }

    if (!dll) {
        LERROR("Null DLL output pointer; failed to load DLL '%s'.", filepath);
        return false;
    }

    HINSTANCE handle = LoadLibraryA(filepath);
    if (!handle) {
        LERROR("Null library handle; cannot load DLL '%s'.", filepath);
        return false;
    }

    dll->filepath       = filepath;
    dll->handle         = (void*)handle;
    dll->loaded_symbols = AL_Array(AL_Symbol, 1);
    return true;
}

b8 AL_UnloadDLL(AL_DLL* dll) {
    if (!dll) {
        LERROR("Cannot unload null DLL.");
        return false;
    }

    if (!FreeLibrary((HMODULE)dll->handle)) {
        LERROR("Cannot free DLL '%s'.", dll->filepath);
        return false;
    }

    assert(dll->loaded_symbols != NULL);

    AL_ForEach(dll->loaded_symbols, i) {
        AL_Symbol* symbol = dll->loaded_symbols + i;
        free((void*)symbol->symname);
        symbol->addr = NULL;
    }

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

    u64 hash = FNV_1A(symname, strlen(symname));

    AL_ForEach(dll->loaded_symbols, i) {
        AL_Symbol* symbol = dll->loaded_symbols + i;
        if (symbol->hash == hash) return symbol;
    }

    void* addr = GetProcAddress((HMODULE)dll->handle, symname);
    if (!addr) {
        if (required) LERROR("Symbol '%s' not found within library '%s'.", symname, dll->filepath);
        else
            LNOTE("Symbol '%s' not found within library '%s'.", symname, dll->filepath);
        return NULL;
    }

    AL_Symbol symbol = { .addr = addr, .symname = symname, .hash = hash };
    AL_Append(dll->loaded_symbols, symbol);

    return AL_Last(dll->loaded_symbols);
}

AL_Symbol* AL_FindSymbol(AL_DLL* dll, const char* symname) {
    if (!dll) {
        LERROR("Cannot find symbol from null DLL.");
        return NULL;
    }

    if (!symname) {
        LERROR("Cannot search DLL '%s' with a null symbol name string.", dll->filepath);
        return NULL;
    }

    u64 hash = FNV_1A(symname, strlen(symname));

    AL_ForEach(dll->loaded_symbols, i) {
        AL_Symbol* symbol = dll->loaded_symbols + i;
        if (symbol->hash == hash) return symbol;
    }

    LERROR("Symbol '%s' not found within DLL '%s'.", symname, dll->filepath);
    return NULL;
}

#endif
