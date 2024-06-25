#include "../include/cache.hpp"

#include <systemc.h>

#include "../include/simulation.h"

#define L1_CACHE_SIZE 4
#define L2_CACHE_SIZE 8
#define CACHE_LINE_SIZE 16

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
    : sc_module(name),
      l1_cache("l1_cache"),
      l2_cache("l2_cache"),
      memory("memory", 1024) {  // how do we handle memory size?
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

  memory.clk(clk);
  memory.reset(reset);
  memory.address(address);
  memory.data_in(data_in);
  memory.we(we);
  memory.data_out(memory_data_out);

  // l1_cache.init(L1_CACHE_SIZE, CACHE_LINE_SIZE, 1);
  // l2_cache.init(L2_CACHE_SIZE, CACHE_LINE_SIZE, 5);

  SC_METHOD(forward_outputs);
  sensitive << l1_hit << l2_hit;
}

void CacheSimulation::init(unsigned l1_cache_size, unsigned l2_cache_size,
                           unsigned cache_line_size, unsigned l1_latency,
                           unsigned l2_latency, unsigned memory_latency) {
  l1_cache.init(l1_cache_size, cache_line_size, l1_latency);
  l2_cache.init(l2_cache_size, cache_line_size, l2_latency);
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
    //data_out.write(0);
    data_out.write(memory_data_out.read());
    
  }
}