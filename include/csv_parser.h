#ifndef CSV_PARSER_H
#define CSV_PARSER_H

#include "error.h"
#include "types.h"

Request *parse_csv(const char *filename, size_t *num_requests);

#endif  // CSV_PARSER_H