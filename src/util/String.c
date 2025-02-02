#include "String.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "log/Logger.h"
#include "Allocation.h"

String newStringEmpty(const size_t initCapacity) {
	const size_t capacity = initCapacity > 0 ? initCapacity : 16;
	String string = { };
	string.pBuffer = heapAlloc(capacity, sizeof(char));
	if (!string.pBuffer) {
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "Creating empty string: failed to allocate string with capacity %llu (requested capacity: %llu).", capacity, initCapacity);
		return (String){ };
	}
	string.capacity = capacity;
	return string;
}

String newString(const size_t capacity, const char *const pInitData) {
	
	String string = newStringEmpty(capacity);
	if (stringIsNull(string) || !pInitData) {
		return string;
	}
	
	const size_t maxLen = string.capacity - 1;	// Maximum possible length of 
	const size_t initLen = strlen(pInitData);	// Length of initial data NOT including null terminator.
	strncpy(string.pBuffer, pInitData, initLen);
	string.length = initLen > maxLen ? maxLen : initLen;
	
	return string;
}

String deepCopyString(const String copy) {
	return newString(copy.capacity, copy.pBuffer);
}

void deleteString(String *const pString) {
	assert(pString);
	pString->pBuffer = heapFree(pString->pBuffer);
	pString->length = 0;
	pString->capacity = 0;
}

bool stringIsNull(const String string) {
	return string.capacity == 0 || string.pBuffer == nullptr;
}

bool stringCompare(const String a, const String b) {
	if (stringIsNull(a) || stringIsNull(b)) {
		return false;
	}
	
	if (a.length != b.length) {
		return false;
	}
	
	return strncmp(a.pBuffer, b.pBuffer, a.length) == 0;
}

size_t stringReverseSearchChar(const String string, const char c) {
	if (stringIsNull(string)) {
		return 0;
	}
	
	const char *reverse_search_result = strrchr(string.pBuffer, c);
	if (reverse_search_result == nullptr) {
		return string.length;
	}
	
	const ptrdiff_t ptr_difference = reverse_search_result - string.pBuffer;
	return (size_t)ptr_difference;
}

bool stringConcatChar(String *const pString, const char c) {
	if (pString == nullptr) {
		return false;
	}
	
	if (stringIsNull(*pString)) {
		return false;
	}
	
	if (pString->length + 1 >= pString->capacity) {
		// TODO - sanitize reallocation.
		pString->pBuffer = heapRealloc(pString->pBuffer, ++pString->capacity, sizeof(char));
	}
	pString->pBuffer[pString->length++] = c;
	pString->pBuffer[pString->length] = '\0';
	
	return true;
}

bool stringConcatString(String *const pDst, const String src) {
	if (pDst == nullptr) {
		return false;
	}
	
	if (stringIsNull(*pDst) || stringIsNull(src)) {
		return false;
	}
	
	const size_t new_length = pDst->length + src.length;
	if (pDst->capacity <= new_length) {
		// Reallocate new_length + 1 bytes for the buffer, for the extra byte at the end.
		pDst->pBuffer = heapRealloc(pDst->pBuffer, new_length + 1, sizeof(char));
	}
	
	strncpy(&pDst->pBuffer[pDst->length], src.pBuffer, src.length);
	pDst->length = new_length;
	
	return true;
}

bool stringConcatCString(String *const pDst, const char *const src_pstring) {
	if (pDst == nullptr || src_pstring == nullptr) {
		return false;
	}
	
	if (stringIsNull(*pDst)) {
		return false;
	}
	
	const size_t src_length = strlen(src_pstring);
	const size_t new_length = pDst->length + src_length;
	if (pDst->capacity <= new_length) {
		// Reallocate new_length + 1 bytes for the buffer, for the extra byte at the end.
		pDst->pBuffer = heapRealloc(pDst->pBuffer, new_length + 1, sizeof(char));
	}
	
	for (size_t i = 0; i < src_length; ++i) {
		const size_t dest_index = pDst->length + i;
		pDst->pBuffer[dest_index] = src_pstring[i];
	}
	pDst->length = new_length;
	pDst->pBuffer[pDst->length] = '\0';
	
	return true;
}

bool stringRemoveTrailingChars(String *const pString, const size_t num_chars) {
	if (pString == nullptr) {
		return false;
	}
	
	if (stringIsNull(*pString) || num_chars > pString->length) {
		return false;
	}
	
	pString->length -= num_chars;
	pString->pBuffer[pString->length] = '\0';
	return true;
}

static size_t exponentiate(const size_t base, const size_t exponent) {
	// Recursive binary exponentiation.
    if (exponent == 0) {
        // X^0 = 1.
        return 1;
    }
    if (exponent == 1) {
        // X^1 = X.
        return base;
    }
    else if ((exponent & 1) == 0) {
        // If N is even, X^N = X^(N/2) * X^(N/2) = (X^2)^(N/2).
        return exponentiate(base * base, exponent / 2);
    }
    else {
        // If N is odd, X^N = X^((N - 1)/2) * X^((N - 1)/2) * X = (X^2)^(N/2) * X.
        return exponentiate(base, exponent - 1) * base;
    }
}

size_t stringHash(const String string, const size_t limit) {
	if (stringIsNull(string)) {
		return 0;
	}
	
	if (limit < 1) {
		return 0;
	}
	
	// Multiplicative string hashing.
	static const size_t p = 32;
	size_t sum = 0;
	for (size_t i = 0; i < string.length; ++i) {
		sum += (size_t)string.pBuffer[i] * exponentiate(p, i);
	}
	
	return sum % limit;
}