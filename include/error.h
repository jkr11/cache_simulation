#ifndef ERROR_H
#define ERROR_H


#include<stdio.h>



#define HANDLE_ERROR(msg) \
	do { \
		fprintf(stderr, "Error: %s\n", msg); \
		exit (EXIT_FAILURE); \
	} while (0)

#define HANDLE_ERROR_FMT(fmt, ...) \
	do { \
		fprintf(stderr, "Error: " fmt "\n", __VA_ARGS__); \
		exit(EXIT_FAILURE); \
	} while (0)


#endif