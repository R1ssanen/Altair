#include "array.h"

#include <math.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>

#include "aldefs.h"
#include "log.h"

void* CreateArray_(u64 stride, u64 count) {
	if (stride == 0) {
		LERROR("Cannot create array with element stride of 0 bytes.");
		return NULL;
	}

	if (count <= 0) count = 1;

	u64* header = malloc(3 * sizeof(u64) + count * stride);
	if (!header) {
		free(header);
		LERROR("Could not allocate %lluB of memory for array.", ARRAY_TAG_END * sizeof(u64) + count * stride);
		return NULL;
	}

	header[ARRAY_TAG_CAPACITY] = count;
	header[ARRAY_TAG_LENGTH] = 0;
	header[ARRAY_TAG_STRIDE] = stride;

	return (void*)(header + ARRAY_TAG_END);
}

void FreeArray_(void* array) {
	assert(array != NULL);
	u64* header = HEADER_(array);
	free(header);
}

void* ResizeArray_(void* array) {
	if (!array) {
		LERROR("Null array pointer; cannot resize.");
		return NULL;
	}

	u64* header = HEADER_(array);
	assert(header != NULL);

	u64 new_capacity = ceil(header[ARRAY_TAG_CAPACITY] * AL_ARRAY_RESIZE_FACTOR);
	u64 total_bytes = ARRAY_TAG_END * sizeof(u64) + new_capacity * header[ARRAY_TAG_STRIDE];

	header = realloc(header, total_bytes);
	if (!header) {
		free(header);
		LERROR("Could not reallocate array with %lluB.", total_bytes);
		return NULL;
	}

	header[ARRAY_TAG_CAPACITY] = new_capacity;
	return (void*)(header + ARRAY_TAG_END);
}

void RemoveArray_(void* array, u64 index) {
	if (!array) {
		LERROR("Cannot remove from null array.");
		return;
	}

	assert(index < AL_Size(array));

	u64* header = HEADER_(array);
	u64 stride = header[ARRAY_TAG_STRIDE];
	u8* element = (u8*)array + index * stride;

	memmove((void*)element, (void*)(element + stride), (AL_Size(array) - index - 1) * stride);
	header[ARRAY_TAG_LENGTH]--;
}

