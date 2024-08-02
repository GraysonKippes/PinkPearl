#ifndef FILE_IO_H_2
#define FILE_IO_H_2

#include <stdio.h>

typedef struct File {
	
	FILE *pStream;
	
	bool readable;
	
	bool writable;
	
	bool binaryMode;
	
	bool updateMode;
	
} File;

#endif	// FILE_IO_H_2