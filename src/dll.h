#ifndef AL_FRONTEND_DLL_H_
#define AL_FRONTEND_DLL_H_

#include "aldefs.h"

typedef struct {
	void* addr;
	const char* symname;
	u64 hash;
} AL_ExportTableEntry;

typedef struct {
	AL_ExportTableEntry* exports;
	void* handle;
	const char* filepath;
} AL_DLL;

b8 AL_LoadDLL(const char* filepath, AL_DLL* dll);

b8 AL_UnloadDLL(AL_DLL* dll);

void* AL_LoadSymbol(AL_DLL* dll, const char* symbol, b8 required);

b8 AL_EnumerateExports(AL_DLL* dll);

void* AL_FindSymbol(AL_ExportTableEntry* exports, const char* name);

#endif
