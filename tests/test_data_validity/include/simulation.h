#ifndef SIMULATION_H
#define SIMULATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include "types.h"

Result run_simulation(int cycles, size_t numRequests, struct Request requests[]);

#ifdef __cplusplus
}
#endif

#endif  // SIMULATION_H