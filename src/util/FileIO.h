#ifndef FILE_IO_H
#define FILE_IO_H

#include <stdio.h>
#include "util/String.h"

#define FMODE_NO_UPDATE	0
#define FMODE_UPDATE	1

#define FMODE_TEXT		0
#define FMODE_BINARY	1

typedef enum FileMode {
	FMODE_READ = 0,
	FMODE_WRITE = 1,
	FMODE_APPEND = 2
} FileMode;

typedef struct File {
	FILE *pStream;
	FileMode mode;
	bool update;
	bool binary;
} File;

File openFile(const char path[], const FileMode mode, const bool update, const bool binary);

void closeFile(File *const pFile);

void fileReadData(const File file, const size_t objectCount, const size_t objectSize, void *const pBuffer);

String fileReadString(const File file, const size_t maxCapacity);

#endif	// FILE_IO_H