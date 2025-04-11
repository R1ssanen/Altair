#include "aldefs.h"
#if defined (AL_PLATFORM_WIN)

#include "dll.h"

#include <assert.h>
#include <malloc.h>
#include <string.h>

#include "array.h"
#include "hash.h"
#include "log.h"

b8 AL_LoadDLL(const char* filepath, AL_DLL* dll) {
	if (!filepath) {
		LERROR("Invalid library filepath; loading failed.");
		return false;
	}

	if (!dll) {
		LERROR("Null DLL output pointer; failed to load DLL '%s'.", filepath);
		return false;
	}

	LINFO("%s", filepath);
	HINSTANCE handle = LoadLibraryA(filepath);
	if (!handle) {
		LERROR("Null library handle; cannot load DLL '%s'.", filepath);
		LERROR("Error: %lu", GetLastError());
		return false;
	}

	dll->filepath = filepath;
	dll->handle = (void*)handle;
	dll->exports = AL_Array(AL_ExportTableEntry, 1);
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

	assert(dll->exports != NULL);
	AL_ForEach(dll->exports, i) free(dll->exports[i].symname);
	AL_Free(dll->exports);

	return true;
}

void* AL_LoadSymbol(AL_DLL* dll, const char* symname, b8 required) {
	if (!symname) {
		LERROR("Cannot load null library symbol.");
		return NULL;
	}

	if (!dll) {
		LERROR("Cannot load symbols from null library.");
		return NULL;
	}

	void* symbol = GetProcAddress((HMODULE)dll->handle, symname);
	if (!symbol) {
		if (required) LERROR("Symbol '%s' not found within library '%s'.", symname, dll->filepath);
		else LNOTE("Symbol '%s' not found within library '%s'.", symname, dll->filepath);
		return NULL;
	}

	return symbol;
}

static BOOL __stdcall EnumSymbolsCallback(PSYMBOL_INFO info, ULONG symbol_size, PVOID userdata) {
	char* name = malloc(info->NameLen + 1);
	if (!name) {
		free(name);
		LERROR("Could not allocate %lluB of memory for DLL export symbol name.", info->NameLen + 1);
		return false;
	}

	strcpy_s(name, info->NameLen + 1, info->Name);

	AL_DLL* dll = userdata;
	AL_ExportTableEntry export = { .addr = AL_LoadSymbol(dll, name, true), .symname = name, .hash = FNV_1A(name, info->NameLen) };

	assert(dll->exports != NULL);
	AL_Append(dll->exports, export);

	return true;
}

b8 AL_EnumerateExports(AL_DLL* dll) {
	if (!dll) {
		LERROR("Cannot enumerate exports of null DLL.");
		return false;
	}

	HANDLE proc = GetCurrentProcess();
	if (!proc) {
		LERROR("Could not obtain current Win32 process.");
		return false;
	}

	if (!SymInitialize(proc, NULL, FALSE)) {
		LERROR("Could not initialize symbol handler for DLL '%s'.", dll->filepath);
		return false;
	}

	assert(dll->filepath != NULL);

	DWORD64 base = SymLoadModuleEx(proc, NULL, dll->filepath, NULL, 0, 0, NULL, SLMFLAG_NO_SYMBOLS);
	if (base == 0) {
		LERROR("Could not load the symbol table of DLL '%s'.", dll->filepath);
		return false;
	}

	if (!SymEnumSymbols(proc, base, "*", EnumSymbolsCallback, (void*)dll)) {
		SymUnloadModule64(proc, base);
		LERROR("Could not enumerate export symbols of DLL '%s'.", dll->filepath);
		return false;
	}

	if (!SymUnloadModule64(proc, base)) {
		LERROR("Could not unload the symbol table of DLL '%s'.", dll->filepath);
		return false;
	}

	return true;
}

void* AL_FindSymbol(AL_ExportTableEntry* exports, const char* name) {
	if (!exports) {
		LERROR("Cannot find symbol from null export table array.");
		return NULL;
	}

	if (!name) {
		LERROR("Cannot find null symbol from export table array.");
		return NULL;
	}

	u64 hash = FNV_1A(name, strlen(name));

	AL_ForEach(exports, i) {
		AL_ExportTableEntry* export = exports + i;
		if (export->hash == hash) { return export->addr; }
	}

	return NULL;
}

#endif