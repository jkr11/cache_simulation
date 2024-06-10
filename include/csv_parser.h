#ifndef CSV_PARSER_H
#define CSV_PARSER_H

#include "types.h"
#include "error.h"

Request* parse_csv(const char *filename, size_t *num_requests);

#endif // CSV_PARSER_H