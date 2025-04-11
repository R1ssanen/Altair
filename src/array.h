#ifndef AL_ARRAY_H_
#define AL_ARRAY_H_

#include <assert.h>
#include <string.h>

#include "aldefs.h"

#define AL_ARRAY_RESIZE_FACTOR 1.50

enum {
	ARRAY_TAG_CAPACITY = 0,
	ARRAY_TAG_LENGTH,
	ARRAY_TAG_STRIDE,
	ARRAY_TAG_END
};

#define HEADER_(array) (((u64*)array) - ARRAY_TAG_END)

void* CreateArray_(u64 stride, u64 count);
void  FreeArray_(void* array);
void* ResizeArray_(void* array);
void  RemoveArray_(void* array, u64 index);

#define AL_Array(type, count) (type*)CreateArray_(sizeof(type), count)

#define AL_Free(array) FreeArray_(array)

#define AL_Clear(array)															\
	do {																		\
		assert((array) != NULL);												\
		memset(array, 0, HEADER_(array)[ARRAY_TAG_STRIDE] * HEADER_(array)[ARRAY_TAG_CAPACITY]);	\
		HEADER_(array)[ARRAY_TAG_LENGTH] = 0;											\
	} while (0)

#define AL_Append(array, item)											\
	do {															\
		assert((array) != NULL);									\
		if (HEADER_(array)[ARRAY_TAG_CAPACITY] <= HEADER_(array)[ARRAY_TAG_LENGTH])	\
			array = ResizeArray_(array); 							\
		(array)[HEADER_(array)[ARRAY_TAG_LENGTH]++] = item;					\
	} while (0)

#define AL_Remove(array, i) RemoveArray_(array, i)

#define AL_Size(array) (HEADER_((array))[ARRAY_TAG_LENGTH])

#define AL_ForEach(array, it) for (u64 it = 0; it < AL_Size(array); ++it)

#endif
