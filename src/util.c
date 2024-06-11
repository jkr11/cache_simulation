#include "../include/util.h"

#include <stdio.h>

#include "../include/types.h"

void print_request(const Request *req) {
  printf("Request:\n");
  printf("\taddr: 0x%08X\n", req->addr);
  printf("\tdata: 0x%08X\n", req->data);
  printf("\twe: %d\n", req->we);
}

void print_result(const Result *res) {
  printf("Result:\n");
  printf("\tcycles: %zu\n", res->cycles);
  printf("\tmisses: %zu\n", res->misses);
  printf("\thits: %zu\n", res->hits);
  printf("\tprimitiveGateCount: %zu\n", res->primitiveGateCount);
}

void print_requests(const Request *requests, size_t num_requests) {
  printf("Request List: (%ld requests)\n", num_requests);
  for (size_t i = 0; i < num_requests; ++i) {
    printf("Request %ld:\n", i);
    print_request(&requests[i]);
  }
}