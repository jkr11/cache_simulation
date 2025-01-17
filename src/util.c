#include "../include/util.h"

#include <stdio.h>

#include "../include/types.h"

void print_aux(const Request* req) {
  printf("\taddr: 0x%08X\n", req->addr);
  printf("\tdata: 0x%08X\n", req->data);
  printf("\twe: %d\n", req->we);
}

void print_request(const Request* req) {
  printf("Request:\n");
  print_aux(req);
}

void print_result(const Result* res) {
  printf("Result:\n");
  printf("\tcycles: %zu\n", res->cycles);
  printf("\tmisses: %zu\n", res->misses);
  printf("\thits: %zu\n", res->hits);
  printf("\tprimitiveGateCount: %zu\n", res->primitiveGateCount);
}

void print_requests(const Request* requests, size_t num_requests) {
  printf("Request List: (%zu requests)\n", num_requests);
  for (size_t i = 0; i < num_requests; ++i) {
    printf("Request %zu:\n", i);
    print_aux(&requests[i]);
  }
}
