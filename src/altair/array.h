#ifndef AL_ARRAY_H_
#define AL_ARRAY_H_

#include <assert.h>
#include <string.h>

#include "aldefs.h"
#include "threads.h"

#define AL_ARRAY_RESIZE_FACTOR 1.50

enum {
	ARRAY_TAG_CAPACITY = 0,
	ARRAY_TAG_LENGTH,
	ARRAY_TAG_STRIDE,
	ARRAY_TAG_END
};

#define HEADER_(array) (((u64*)array) - ARRAY_TAG_END)

ALAPI void* CreateArray_(u64 stride, u64 count);
ALAPI void* ResizeArray_(void* array);

ALAPI void  AL_Remove(void* array, u64 index);
ALAPI void  AL_Free(void* array);
ALAPI void  AL_Clear(void* array);

#define AL_Array(type, count) (type*)CreateArray_(sizeof(type), count)

#define AL_Append(array, item)														\
	do {																			\
		assert((array) != NULL);													\
		if (HEADER_(array)[ARRAY_TAG_CAPACITY] <= HEADER_(array)[ARRAY_TAG_LENGTH])	\
			array = ResizeArray_(array); 											\
		(array)[HEADER_(array)[ARRAY_TAG_LENGTH]++] = item;							\
	} while (0)

#define AL_Size(array) (HEADER_(array)[ARRAY_TAG_LENGTH])

#define AL_ForEach(array, it) for (u64 it = 0; it < AL_Size(array); ++it)

#define AL_Last(array) (array + (HEADER_(array)[ARRAY_TAG_LENGTH] - 1))

#endif
