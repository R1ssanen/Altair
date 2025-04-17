#ifndef AL_STRING_H_
#define AL_STRING_H_

#include "aldefs.h"
#include "array.h"

typedef char* AL_String; // array

#define AL_Str(len) AL_Array(char, len)

#define AL_Add(str, ch)                                                                            \
    do {                                                                                           \
        if (*AL_Last(str) == '\0') AL_Size(str)--;                                                 \
        AL_Append(str, ch);                                                                        \
        *AL_Metadata(str) = 0;                                                                     \
    } while (0)

ALAPI AL_String AL_CopyC(const char* src, u64 len);

#define AL_Copy(str) AL_CopyC(str, AL_Size(str))

ALAPI AL_String AL_ConcatC(AL_String lhs, const char* rhs, u64 rhs_len);

#define AL_Concat(lhs, rhs) AL_ConcatC(lhs, rhs, AL_Size(rhs))

ALAPI b8 AL_Equals(AL_String a, AL_String b);

#endif
