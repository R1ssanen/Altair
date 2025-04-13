#ifndef AL_FRONTEND_DLL_H_
#define AL_FRONTEND_DLL_H_

#include "aldefs.h"

typedef struct AL_Symbol_ {
	void* addr;
	const char* symname;
	u64 hash;
} AL_Symbol;

typedef struct AL_DLL_ {
	AL_Symbol* loaded_symbols;
	void* handle;
	const char* filepath;
} AL_DLL;

ALAPI b8 AL_LoadDLL(const char* filepath, AL_DLL* dll);

ALAPI b8 AL_UnloadDLL(AL_DLL* dll);

AL_Symbol* AL_LoadSymbol(AL_DLL* dll, const char* symname, b8 required);

//ALAPI b8 AL_EnumerateExports(AL_DLL* dll);

AL_Symbol* AL_FindSymbol(AL_DLL* dll, const char* name);

#endif
