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

  sim.init(l1CacheLines, l2CacheLines, cacheLineSize, l1CacheLatency,
           l2CacheLatency, memoryLatency);

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

    sc_start(1, SC_NS);  // Start the clock cycle for the current request

    if (hit.read()) {
      hits++;
      total_cycles += l1CacheLatency;  // Hit in L1 cache
      std::cout << "L1 cache hit at address 0x" << std::hex << requests[i].addr
                << std::endl;
    } else if (sim.l2_cache.hit.read()) {
      hits++;
      total_cycles +=
          (l1CacheLatency + l2CacheLatency);  // Miss in L1, hit in L2
      std::cout << "L2 cache hit at address 0x" << std::hex << requests[i].addr
                << std::endl;
    } else {
      misses++;
      total_cycles += (l1CacheLatency + l2CacheLatency +
                       memoryLatency);  // Miss in both L1 and L2, hit in memory
      std::cout << "Memory hit at address 0x" << std::hex << requests[i].addr
                << std::endl;
    }

    // If needed for debugging:
    if (!we.read()) {
      std::cout << "Read Address: 0x" << std::hex << address.read()
                << " Data: " << std::dec << data_out.read() << std::endl;
    } else {
      std::cout << "Write Address: 0x" << std::hex << address.read()
                << " Data: " << std::dec << data_in.read() << std::endl;
    }
  }

  Result result;
  result.cycles = total_cycles;
  result.misses = misses;
  result.hits = hits;
  result.primitiveGateCount = 0;

  return result;
}

int sc_main(int argc, char* argv[]) { return 0; }
