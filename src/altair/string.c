#include "string.h"

#include "aldefs.h"
#include "array.h"
#include "hash.h"
#include "log.h"

AL_String AL_CopyC(const char* src, u64 len) {
    if (!src) {
        LERROR("Cannot copy a null string.");
        return NULL;
    }

    assert(len > 0);

    AL_String new_str = AL_Str(len + 1);
    strncpy(new_str, src, len + 1);

    AL_Size(new_str)      = len;
    *AL_Metadata(new_str) = 0;
    return new_str;
}

AL_String AL_ConcatC(AL_String lhs, const char* rhs, u64 rhs_len) {
    if (!lhs || !rhs) {
        LERROR("Cannot catenate a null string.");
        return NULL;
    }

    u64 new_len = AL_Size(lhs) + rhs_len + 1;

    AL_Resize(lhs, new_len + 1);
    strncat(lhs, rhs, rhs_len + 1);
    AL_Size(lhs)      = new_len;

    *AL_Metadata(lhs) = 0;
    return lhs;
}

b8 AL_Equals(AL_String a, AL_String b) {
    u64* hash_a = AL_Metadata(a);
    if (hash_a == 0) *hash_a = FNV_1A(a);

    u64* hash_b = AL_Metadata(b);
    if (hash_b == 0) *hash_a = FNV_1A(b);

    return hash_a == hash_b;
}
