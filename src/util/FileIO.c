#include "FileIO.h"

#include <assert.h>
#include <errno.h>
#include "log/Logger.h"

File openFile(const char path[], const FileMode mode, const bool update, const bool binary) {
	
	// Build mode-specification string.
	
	char modeString[8];
	int modeStringPos = 0;
	
	switch (mode) {
		case FMODE_READ:
			modeString[modeStringPos] = 'r';
			break;
		case FMODE_WRITE:
			modeString[modeStringPos] = 'w';
			break;
		case FMODE_APPEND:
			modeString[modeStringPos] = 'a';
			break;
	}
	modeStringPos += 1;
	
	if (update) {
		modeString[modeStringPos] = '+';
		modeStringPos += 1;
	}
	
	if (binary) {
		modeString[modeStringPos] = 'b';
		modeStringPos += 1;
	}
	
	if (mode == FMODE_WRITE) {
		modeString[modeStringPos] = 'x';
		modeStringPos += 1;
	}
	
	// Open the actual file itself.
	
	File file = {
		.pStream = nullptr,
		.mode = mode,
		.update = update,
		.binary = binary
	};
	
	const errno_t result = fopen_s(&file.pStream, path, modeString);
	if (result != 0) {
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "Opening file: failed to open file \"%s\" (error code = \"%s\").", path, strerror(result));
		return (File){ };
	}
	
	return file;
}

void closeFile(File *const pFile) {
	assert(pFile);
	if (fclose(pFile->pStream) != 0) {
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "Closing file: failed to close file.");
		return;
	}
	*pFile = (File){ };
}

void fileReadData(const File file, const size_t objectCount, const size_t objectSize, void *const pBuffer) {
	if (!file.pStream) {
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "Reading data from file: file stream is null.");
		return;
	}
	
	if (objectCount == 0 || objectSize == 0) {
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "Reading data from file: invalid data size requested (object count: %llu, object size: %llu).", objectCount, objectSize);
		return;
	}
	
	if (!pBuffer) {
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "Reading data from file: destination buffer is null.");
		return;
	}
	
	const size_t bytesToRead = objectCount * objectSize;
	const size_t bytesRead = fread(pBuffer, objectSize, objectCount, file.pStream);
	if (bytesRead != bytesToRead) {
		logMsg(loggerSystem, LOG_LEVEL_WARNING, "Reading data from file: did not read requested number of bytes (requested: %llu, actually read: %llu).", bytesToRead, bytesRead);
	}
}

String fileReadString(const File file, const size_t maxCapacity) {
	if (!file.pStream) {
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "Reading string from file: file stream is null.");
		return (String){ };
	}
	
	const long int currentPosition = ftell(file.pStream);
	if (currentPosition < 0) {
		const errno_t error = errno;
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "Reading string from file: failed to get file position (error code = \"%s\").", strerror(error));
		return (String){ };
	}
	
	// First pass: get total length of string in file.
	size_t stringLength = 0;
	int ch = fgetc(file.pStream);
	while (ch != '\0' && ch != EOF && stringLength < maxCapacity) {
		stringLength += 1;
		ch = fgetc(file.pStream);
	}
	size_t stringCapacity = stringLength + 1;
	
	// Seek back to initial position.
	const int seekResult = fseek(file.pStream, currentPosition, SEEK_SET);
	if (seekResult != 0) {
		const errno_t error = errno;
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "Reading string from file: failed to seek to start of string (error code = \"%s\").", strerror(error));
		return (String){ };
	}
	
	// Second pass: read characters from file into string buffer.
	String string = newStringEmpty(stringCapacity);
	if (stringIsNull(string)) {
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "Reading string from file: failed to allocate string.");
		return (String){ };
	}
	size_t counter = 0;
	ch = fgetc(file.pStream);
	while (ch != '\0' && ch != EOF && counter < stringCapacity) {
		string.pBuffer[counter++] = (char)ch;
		ch = fgetc(file.pStream);
	}
	string.pBuffer[counter] = '\0';
	string.length = stringLength;
	return string;
}