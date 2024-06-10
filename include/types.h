#ifndef TYPES_H
#define TYPES_H

#include<stdint.h> 
#include<stddef.h> // for size_t 

typedef struct Request {
	uint32_t addr;
	uint32_t data;
	int we; // 0 read 1 write
} Request;

typedef struct Result {
	size_t cycles;
	size_t misses;
	size_t hits; 
	size_t primitiveGateCount;
} Result;

#endif // TYPES_H
