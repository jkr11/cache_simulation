#include "../include/cache.hpp"

#include <systemc.h>

#include "../include/simulation.h"

#define L1_CACHE_SIZE 4
#define L2_CACHE_SIZE 8
#define CACHE_LINE_SIZE 16

/*
SC_MODULE(Cache) {
  sc_in<bool> clk;
  sc_in<bool> reset;
  sc_in<uint32_t> address;
  sc_in<uint32_t> data_in;
  sc_in<bool> we;
  sc_out<uint32_t> data_out;
  sc_out<bool> hit;

  int size;
  int line_size;
  int latency;
  uint32_t* tags;
  uint32_t* data;
  int* lru;

  SC_HAS_PROCESS(Cache);

*/
Cache::Cache(sc_module_name name)
    : sc_module(name),
      size(0),
      line_size(0),
      latency(0),
      tags(nullptr),
      data(nullptr),
      lru(nullptr) {
  SC_METHOD(cache_access);
  sensitive << clk.pos();
  dont_initialize();
}

Cache::~Cache() {
  if (tags) delete[] tags;
  if (data) delete[] data;
  if (lru) delete[] lru;
}

void Cache::init(int size, int line_size, int latency) {
  this->size = size;
  this->line_size = line_size;
  this->latency = latency;

  tags = new uint32_t[size];
  data = new uint32_t[size * line_size];
  lru = new int[size];

  for (int i = 0; i < size; ++i) {
    tags[i] = -1;
    lru[i] = i;
  }
  for (int i = 0; i < line_size * size; ++i) {
    data[i] = 0;
  }
}

bool Cache::access(uint32_t address, uint32_t& out_data, uint32_t in_data,
                   bool is_write) {
  uint32_t tag = address >> (2 + (int)log2(line_size));
  uint32_t index = (address >> 2) & (size - 1);
  uint32_t offset = address & ((1 << 2) - 1);

  if (tags[index] == tag) {
    if (is_write) {
      data[index * line_size + offset] = in_data;
    }
    out_data = data[index * line_size + offset];
    update_lru(index);
    return true;
  } else {
    int lru_index = lru[size - 1];
    tags[lru_index] = tag;
    if (is_write) {
      data[lru_index * line_size + offset] = in_data;
    }
    out_data = data[lru_index * line_size + offset];
    update_lru(lru_index);
    return false;
  }
}

void Cache::update_lru(int acc_index) {
  for (int i = 0; i < size; ++i) {
    if (lru[i] == acc_index) {
      for (int j = i; j > 0; --j) {
        lru[j] = lru[j - 1];
      }
      lru[0] = acc_index;
      break;
    }
  }
}

void Cache::cache_access() {
  if (reset.read()) {
    for (int i = 0; i < size; ++i) {
      tags[i] = -1;
      for (int j = 0; j < line_size; ++j) {
        data[i * line_size + j] = 0;
      }
      lru[i] = i;
    }
  } else {
    uint32_t out_data = 0;
    bool hit_s = access(address.read(), out_data, data_in.read(), we.read());
    data_out.write(out_data);
    hit.write(hit_s);
  }
}

CacheSimulation::CacheSimulation(sc_module_name name)
    : sc_module(name), l1_cache("l1_cache"), l2_cache("l2_cache") {
  l1_cache.clk(clk);
  l1_cache.reset(reset);
  l1_cache.address(address);
  l1_cache.data_in(data_in);
  l1_cache.we(we);
  l1_cache.data_out(l1_data_out);
  l1_cache.hit(l1_hit);

  l2_cache.clk(clk);
  l2_cache.reset(reset);
  l2_cache.address(address);
  l2_cache.data_in(data_in);
  l2_cache.we(we);
  l2_cache.data_out(l2_data_out);
  l2_cache.hit(l2_hit);

  l1_cache.init(L1_CACHE_SIZE, CACHE_LINE_SIZE, 1);
  l2_cache.init(L2_CACHE_SIZE, CACHE_LINE_SIZE, 5);

  SC_METHOD(forward_outputs);
  sensitive << l1_hit << l2_hit;
}

void CacheSimulation::init(unsigned l1_cache_size, unsigned l2_cache_size,
                           unsigned cache_line_size) {
  l1_cache.init(l1_cache_size, cache_line_size, 1);
  l2_cache.init(l2_cache_size, cache_line_size, 5);
}

void CacheSimulation::forward_outputs() {
  if (l1_hit.read()) {
    data_out.write(l1_data_out.read());
    hit.write(true);
  } else if (l2_hit.read()) {
    data_out.write(l2_data_out.read());
    hit.write(true);
  } else {
    hit.write(false);
    data_out.write(0);
  }
}

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
  std::cout << "Cycles: " << result.cycles << " Hits: " << result.hits
            << " Misses: " << result.misses << std::endl;

  return 0;
}