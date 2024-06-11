#include "../include/csv_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/error.h"

Request *parse_csv(const char *filename, size_t *num_requests) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    HANDLE_ERROR_FMT("Could not open file %s", filename);
  }
  char line[256];
  size_t capacity = 10;
  *num_requests = 0;
  Request *requests = (Request *)malloc(capacity * sizeof(Request));
  if (!requests) {
    HANDLE_ERROR("malloc requests");
  }

  int ln = 0;
  while (fgets(line, sizeof(line), file)) {
    ln++;
    if (*num_requests >= capacity) {
      capacity *= 2;
      requests = (Request *)realloc(requests, capacity * sizeof(Request));
      if (!requests) {
        HANDLE_ERROR("Realloc failed");
      }
    }
    Request *req = &requests[*num_requests];
    char *type_str = strtok(line, ",");
    char *address_str = strtok(NULL, ",");
    char *value_str = strtok(NULL, ",");

    if (!type_str || !address_str || (type_str[0] == 'W' && !value_str)) {
      HANDLE_ERROR_FMT("Invalid format on line %d\n", ln);
    }
    req->we = (type_str[0] == 'W') ? 1 : 0;
    req->addr = strtoul(address_str, NULL, 0);
    req->data = (value_str) ? strtoul(value_str, NULL, 0) : 0;

    if (req->we != 0 && req->we != 1) {
      HANDLE_ERROR_FMT("Invalid request type on line %d\n", ln);
    }
    (*num_requests)++;
  }
  fclose(file);
  return requests;
}
