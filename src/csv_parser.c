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
  char line[256];  // assuming all lines are vald, technically all lines are
                   // only of length 3
  size_t capacity = 10;  // here we can choose any nonzero capacity
  *num_requests = 0;
  Request *requests = (Request *)malloc(capacity * sizeof(Request));
  // free before end
  if (!requests) {
    fclose(file);
    HANDLE_ERROR("malloc requests");
  }

  int ln = 0;
  while (fgets(line, sizeof(line), file)) {
    ln++;
    if (*num_requests >= capacity) {
      capacity *= 2;
      // I would recommend that we use a new pointer for reallocation
      // by failure we need to free the space and exit. We would lose the
      // reference to the original space and therefore have a memory leak
      Request *newRequests =
          (Request *)realloc(requests, capacity * sizeof(Request));
      if (!newRequests) {
        fclose(file);
        free(requests);
        HANDLE_ERROR("Realloc failed");
      }
      requests = newRequests;
    }
    Request *req = &requests[*num_requests];
    char *type_str = strtok(line, ",");
    char *address_str = strtok(NULL, ",");
    char *value_str =
        strtok(NULL, ",");  // This might be null, if there is a trailing comma
                            // in the csv this is a newline

    // same problem here, we have to free it before it ends

    if (type_str == NULL || address_str == NULL) {
      fclose(file);
      free(requests);
      HANDLE_ERROR_FMT("Invalid format on line %d\n", ln);
    }

    if (type_str[0] == '\0' || type_str[1] != '\0') {
      fclose(file);
      free(requests);
      HANDLE_ERROR_FMT("Invalid or empty type on line %d\n", ln);
    }

    if (type_str[0] == 'W') {
      if (value_str == NULL || *value_str == '\0' ||
          *value_str == '\n') {  // Handle empty string case
        fclose(file);
        free(requests);
        HANDLE_ERROR_FMT("Invalid format on line %d\n", ln);
      }
    } else if (type_str[0] != 'R') {// type_str isnt 'R' or 'W'
      fclose(file);
      free(requests);
      HANDLE_ERROR_FMT("re not R or W format on line %d\n", ln);
    }

    req->we = (type_str[0] == 'W') ? 1 : 0;
    req->addr = strtoul(address_str, NULL, 0);
    req->data = (value_str) ? strtoul(value_str, NULL, 0) : 0;
    (*num_requests)++;
  }
  fclose(file);
  return requests;
}
