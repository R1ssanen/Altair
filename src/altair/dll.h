#ifndef AL_FRONTEND_DLL_H_
#define AL_FRONTEND_DLL_H_

#include "aldefs.h"
#include "string.h"

typedef struct AL_Symbol_ {
    void*     addr;
    AL_String symname;
} AL_Symbol;

typedef struct AL_DLL_ {
    AL_Symbol* loaded_symbols;
    void*      handle;
    AL_String  filepath;
} AL_DLL;

b8         AL_LoadDLL(const char* filepath, AL_DLL* dll);

b8         AL_UnloadDLL(AL_DLL* dll);

AL_Symbol* AL_LoadSymbol(AL_DLL* dll, const char* symname, b8 required);

AL_Symbol* AL_FindSymbol(AL_DLL* dll, const char* name, b8 required);

#endif
