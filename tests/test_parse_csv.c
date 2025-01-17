#include <stdio.h>
#include <stdlib.h>

#include "../include/csv_parser.h"
#include "../include/types.h"
#include "../include/util.h"

int main() {
  const char *filename = "faulty_csv.csv";
  size_t num_requests;
  Request *requests = parse_csv(filename, &num_requests);

  printf("Parsed requests:\n");
  for (size_t i = 0; i < num_requests; ++i) {
    printf("%ld: ", i);
    print_request(&requests[i]);
  }
  free(requests);
  return 0;
}
