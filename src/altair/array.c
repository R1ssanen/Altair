#include "array.h"

#include <assert.h>
#include <malloc.h>
#include <math.h>
#include <string.h>

#include "aldefs.h"
#include "log.h"

void* CreateArray_(u64 stride, u64 count) {
    if (stride == 0) {
        LERROR("Cannot create array with element stride of 0 bytes.");
        return NULL;
    }

    if (count <= 0) count = 1;

    u64* header = malloc(ARRAY_END * sizeof(u64) + count * stride);
    if (!header) {
        free(header);
        LERROR(
            "Could not allocate %lluB of memory for array.",
            ARRAY_END * sizeof(u64) + count * stride
        );
        return NULL;
    }

    header[ARRAY_SIZE]     = 0;
    header[ARRAY_METADATA] = 0;
    header[ARRAY_CAPACITY] = count;
    header[ARRAY_STRIDE]   = stride;

    return (void*)(header + ARRAY_END);
}

void* ResizeArray_(void* array, u64 new_size) {
    if (!array) {
        LERROR("Null array pointer; cannot resize.");
        return NULL;
    }

    u64* header = HEADER_(array);
    assert(header != NULL);

    u64 new_capacity = new_size ? new_size : ceil(header[ARRAY_CAPACITY] * AL_ARRAY_RESIZE_FACTOR);
    u64 total_bytes  = ARRAY_END * sizeof(u64) + new_capacity * header[ARRAY_STRIDE];

    header           = realloc(header, total_bytes);
    if (!header) {
        free(header);
        LERROR("Could not reallocate array with %lluB.", total_bytes);
        return NULL;
    }

    header[ARRAY_CAPACITY] = new_capacity;
    return (void*)(header + ARRAY_END);
}

void AL_Free(void* array) {
    if (!array) return;

    u64* header = HEADER_(array);
    free(header);
}

void AL_Remove(void* array, u64 index) {
    if (!array) {
        LERROR("Cannot remove from null array.");
        return;
    }

    assert(index < AL_Size(array));

    u64* header  = HEADER_(array);
    u64  stride  = header[ARRAY_STRIDE];
    u8*  element = (u8*)array + index * stride;

    memmove((void*)element, (void*)(element + stride), (AL_Size(array) - index - 1) * stride);
    header[ARRAY_SIZE] -= 1;
}

void AL_Clear(void* array) {
    if (!array) {
        LERROR("Cannon clear null array.");
        return;
    }

    u64* header = HEADER_(array);
    memset(array, 0, header[ARRAY_STRIDE] * header[ARRAY_CAPACITY]);
    header[ARRAY_SIZE] = 0;

    return;
}
