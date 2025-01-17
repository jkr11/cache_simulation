#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

#include "types.h"

void print_aux(const Request* req);

void print_request(const Request* req);

void print_result(const Result* res);

void print_requests(const Request* requests, size_t num_requests);

#endif  // UTIL_H
