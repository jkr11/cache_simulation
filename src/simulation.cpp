#include "../include/simulation.h"

#include "../include/types.h"

Result run_simulation(int cycles, unsigned l1CacheLines, unsigned l2CacheLines,
                      unsigned cacheLineSize, unsigned l1CacheLatency,
                      unsigned l2CacheLatency, unsigned memoryLatency,
                      size_t numRequests, struct Request requests[],
                      const char *tracefile) {
  return {0, 0, 0, 0};
}