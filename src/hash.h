#ifndef HASH_H_
#define HASH_H_

#include "log.h"
#include "aldefs.h"

static inline u64 FNV_1A(const char* str, u64 len) {
	if (!str) {
		LERROR("Cannot hash (FNV_1A) a null string.");
		return 0;
	}

	if (len == 0) {
		LERROR("Cannot hash (FNV_1A) a string of length 0.");
		return 0;
	}

	u64 hash = 0xcbf29ce484222325;

	for (u64 i = 0; i < len; ++i) {
		hash ^= str[i];
		hash *= 0x100000001b3;
	}

	return hash;
}

#endif
