#ifndef UTIL_STRING_H
#define UTIL_STRING_H

#include <stdbool.h>
#include <stddef.h>

typedef struct String {
	
	// Number of characters in the string.
	// Does not include the null-terminator.
	// Must be on range of [0, capacity).
	size_t length;
	
	// Total size of character buffer in bytes.
	size_t capacity;
	
	// Character buffer containing the string's contents.
	char *buffer;
	
} String;

String makeNullString(void);
String newStringEmpty(const size_t capacity);
String newString(const size_t capacity, const char *const initData);
bool deleteString(String *const pString);

// String analysis
bool stringIsNull(const String string);
bool stringCompare(const String a, const String b);
size_t stringReverseSearchChar(const String string, const char c);

// String mutation
bool stringConcatChar(String *const pString, const char c);
bool stringConcatString(String *const pDst, const String src);
bool stringConcatCStr(String *const pDst, const char *const pSrc);
bool stringRemoveTrailingChars(String *const pString, const size_t numChars);

// Miscellaneous
size_t stringHash(const String string, const size_t limit);

#endif	// UTIL_STRING_H