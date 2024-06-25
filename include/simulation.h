#ifndef SIMULATION_H
#define SIMULATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include "types.h"

Result run_simulation(int cycles, unsigned l1CacheLines, unsigned l2CacheLines,
                      unsigned cacheLineSize, unsigned l1CacheLatency,
                      unsigned l2CacheLatency, unsigned memoryLatency,
                      size_t numRequests, struct Request requests[],
                      const char *tracefile);

#ifdef __cplusplus
}
#endif

#endif  // SIMULATION_H