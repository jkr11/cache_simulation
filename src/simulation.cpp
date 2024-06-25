#include "../include/simulation.h"

#include "../include/cache.hpp"
#include "../include/types.h"
#include "../include/util.h"

Result run_simulation(int cycles, unsigned l1CacheLines, unsigned l2CacheLines,
                      unsigned cacheLineSize, unsigned l1CacheLatency,
                      unsigned l2CacheLatency, unsigned memoryLatency,
                      size_t numRequests, struct Request requests[],
                      const char* tracefile) {
  (void)tracefile;

  sc_signal<bool> reset;
  sc_signal<uint32_t> address;
  sc_signal<uint32_t> data_in;
  sc_signal<uint32_t> data_out;
  sc_signal<bool> we;
  sc_signal<bool> hit;

  sc_clock clk("clk", 1, SC_NS);

  CacheSimulation sim("sim");
  sim.clk(clk);
  sim.reset(reset);
  sim.address(address);
  sim.data_in(data_in);
  sim.we(we);
  sim.data_out(data_out);
  sim.hit(hit);

  sim.init(l1CacheLines, l2CacheLines, cacheLineSize);

  sc_start(0, SC_NS);
  reset = true;
  sc_start(2, SC_NS);
  reset = false;

  size_t hits = 0;
  size_t misses = 0;
  size_t total_cycles = 0;

  for (size_t i = 0; i < numRequests; ++i) {
    address = requests[i].addr;
    data_in = requests[i].data;
    we = requests[i].we;

    sc_start(1, SC_NS);

    bool l1_hit = hit.read();
    bool l2_hit = !l1_hit && sim.hit.read();

    if (l1_hit) {
      hits++;
      total_cycles += 1;
    } else if (l2_hit) {
      hits++;
      total_cycles += 6;
    } else {
      misses++;
      total_cycles += 11;
    }

    if (!we.read()) {
      std::cout << "Read Address: " << address.read()
                << " Data: " << data_out.read() << std::endl;
    } else {
      std::cout << "Write Address: " << address.read()
                << " Data: " << data_in.read() << std::endl;
    }
  }

  Result result;
  result.cycles = total_cycles;
  result.misses = misses;
  result.hits = hits;
  result.primitiveGateCount = 0;

  return result;
}

int sc_main(int argc, char* argv[]) {
  Request requests[] = {
      {0x0000, 0xABCD, 0}, {0x0010, 0x1234, 1}, {0x0020, 0x5678, 0},
      {0x0030, 0x9ABC, 0}, {0x0040, 0xDEF0, 1},
  };

  unsigned l1_size = 4;
  unsigned l2_size = 8;
  unsigned cache_line_size = 16;

  Result result = run_simulation(100, l1_size, l2_size, cache_line_size, 1, 5,
                                 11, 5, requests, nullptr);
  print_result(&result);
  // std::cout << "Cycles: " << result.cycles << " Hits: " << result.hits
  //           << " Misses: " << result.misses << std::endl;

  return 0;
}