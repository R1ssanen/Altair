#ifndef AL_ARRAY_H_
#define AL_ARRAY_H_

#include <assert.h>
#include <string.h>

#include "aldefs.h"
#include "threads.h"

#define AL_ARRAY_RESIZE_FACTOR 1.50

enum {
    ARRAY_CAPACITY = 0,
    ARRAY_SIZE,
    ARRAY_STRIDE,
    ARRAY_METADATA, // user data
    ARRAY_END,
};

#define HEADER_(array) (((u64*)array) - ARRAY_END)

ALAPI void* CreateArray_(u64 stride, u64 count);
ALAPI void* ResizeArray_(void* array, u64 new_size);

ALAPI void  AL_Remove(void* array, u64 index);
ALAPI void  AL_Free(void* array);
ALAPI void  AL_Clear(void* array);

#define AL_Array(type, count) (type*)CreateArray_(sizeof(type), count)

#define AL_Resize(array, new_size)                                                                 \
    do { array = ResizeArray_(array, new_size); } while (0)

#define AL_Size(array)     HEADER_(array)[ARRAY_SIZE]

#define AL_Capacity(array) HEADER_(array)[ARRAY_CAPACITY]

#define AL_Stride(array)   HEADER_(array)[ARRAY_STRIDE]

#define AL_Metadata(array) (HEADER_(array) + ARRAY_METADATA)

#define AL_Append(array, item)                                                                     \
    do {                                                                                           \
        assert((array) != NULL);                                                                   \
        if (AL_Capacity(array) <= AL_Size(array)) array = ResizeArray_(array, 0);                  \
        (array)[AL_Size(array)++] = item;                                                          \
    } while (0)

#define AL_ForEach(array, it) for (u64 it = 0; it < AL_Size(array); ++it)

#define AL_Last(array)        (array + AL_Size(array) - 1)

#endif
