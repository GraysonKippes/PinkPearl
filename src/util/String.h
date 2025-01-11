#ifndef UTIL_STRING_H
#define UTIL_STRING_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define makeConstantString(cstr) \
		{ \
			.length = sizeof(cstr) - 1, \
			.capacity = sizeof(cstr), \
			.pBuffer = cstr, \
		}

// Constructs a String out of a static c-string, AKA char[].
#define makeStaticString(cstr) \
		(const String){ \
			.length = sizeof(cstr) - 1, \
			.capacity = sizeof(cstr), \
			.pBuffer = cstr, \
		}

// TODO: consider adding a flag for whether the string is malloced/freeable or not.
typedef struct String {
	
	// Number of characters in the string.
	// Does not include the null-terminator.
	// Must be on range of [0, capacity).
	size_t length;
	
	// Total size of character buffer in bytes.
	size_t capacity;
	
	// Character buffer containing the string's contents.
	char *pBuffer;
	
} String;

typedef union StringParam {
	String string;
	char *pCString;
} StringParam;

// Returns a new string with no contents.
String newStringEmpty(const size_t capacity);

// Returns a new string with the specified initial data.
String newString(const size_t capacity, const char *const initData);

// Returns a new string with the same data as the given string.
String deepCopyString(const String copy);

// Frees the memory held by the string.
void deleteString(String *const pString);

// String analysis
bool stringIsNull(const String string);
bool stringCompare(const String a, const String b);
size_t stringReverseSearchChar(const String string, const char c);

// String mutation
bool stringConcatChar(String *const pString, const char c);
bool stringConcatString(String *const pDst, const String src);
bool stringConcatCString(String *const pDst, const char *const pSrc);
bool stringRemoveTrailingChars(String *const pString, const size_t numChars);

// Miscellaneous
size_t stringHash(const String string, const size_t limit);

#endif	// UTIL_STRING_H