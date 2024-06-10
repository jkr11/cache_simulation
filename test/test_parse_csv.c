#include<stdio.h>
#include<stdlib.h>
#include "../include/csv_parser.h"
#include "../include/types.h"

int main() {
	const char * filename = "example.csv";
	size_t num_requests;
	Request *requests = parse_csv(filename, &num_requests);

	printf("Parsed requests:\n");
	for(size_t i = 0; i < num_requests; ++i) {
		printf("Request %zu: addr=%x, data=%x, we=%d\n", i, requests[i].addr, requests[i].data, requests[i].we);
	}
	free(requests);
	return 0;
}
