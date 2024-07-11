#include "../include/cache.hpp"

#include <systemc.h>

#include <cmath>
#include <iostream>

#include "../include/simulation.h"

Cache::Cache(sc_module_name name)
    : MemoryInterface(name),
      size(0),
      line_size(0),
      latency(0),
      tags(nullptr),
      data(nullptr),
      next_level_memory(nullptr) {
  SC_THREAD(cache_access);
  sensitive << clk.pos();
  dont_initialize();
}

Cache::~Cache() {
  if (tags) delete[] tags;
  if (data) delete[] data;
}

void Cache::init(int size, int line_size, int latency) {
  this->size = size;
  this->line_size = line_size;
  this->latency = latency;

  tags = new uint32_t[size];
  data = new uint32_t[size * line_size];

  for (int i = 0; i < size; ++i) {
    tags[i] = -1;
  }
}

bool Cache::access(uint32_t address, uint32_t& out_data, uint32_t in_data,
                   bool is_write) {
  uint32_t tag = address >> (2 + (int)log2(line_size));
  uint32_t index = (address >> (int)log2(line_size)) & (size - 1);
  uint32_t offset = address & (line_size - 1);

  if (tags[index] == tag) {
    // Cache hit
    if (is_write) {
      data[index * line_size + offset] = in_data;
      std::cout << "Cache hit when writing to address 0x" << std::hex << address
                << " with data " << std::dec << in_data << std::endl;
    }
    out_data = data[index * line_size + offset];
    std::cout << "Cache hit when reading from address 0x" << std::hex << address
              << " with data " << std::dec << out_data << std::endl;

    if (is_write && next_level_memory) {
      next_level_memory->access(address, out_data, in_data, true);
    }
    return true;
  } else {
    // Cache miss
    if (next_level_memory) {
      next_level_memory->access(address, out_data, in_data, is_write);
    }
    if (is_write) {
      data[index * line_size + offset] = in_data;
      std::cout << "Cache miss when writing to address 0x" << std::hex
                << address << " with data " << std::dec << in_data << std::endl;
    } else {
      data[index * line_size + offset] = out_data;
      std::cout << "Cache miss when reading from address 0x" << std::hex
                << address << " with data " << std::dec << out_data
                << std::endl;
    }
    tags[index] = tag;
    return false;
  }
}

void Cache::cache_access() {
  while (true) {
    wait();  // Wait for the clock edge

    if (reset.read()) {
      for (int i = 0; i < size; ++i) {
        tags[i] = -1;
        for (int j = 0; j < line_size; ++j) {
          data[i * line_size + j] = 0;
        }
      }
    } else {
      uint32_t out_data = 0;
      bool hit_s = access(address.read(), out_data, data_in.read(), we.read());
      data_out.write(out_data);
      hit.write(hit_s);

      wait(latency, SC_NS);  // Wait for the latency
    }
  }
}

CacheSimulation::CacheSimulation(sc_module_name name)
    : sc_module(name),
      l1_cache("l1_cache"),
      l2_cache("l2_cache"),
      memory("memory") {
  l1_cache.clk(clk);
  l1_cache.reset(reset);
  l1_cache.address(address);
  l1_cache.data_in(data_in);
  l1_cache.we(we);
  l1_cache.data_out(l1_data_out);
  l1_cache.hit(l1_hit);
  l1_cache.next_level_memory = &l2_cache;

  l2_cache.clk(clk);
  l2_cache.reset(reset);
  l2_cache.address(address);
  l2_cache.data_in(data_in);
  l2_cache.we(we);
  l2_cache.data_out(l2_data_out);
  l2_cache.hit(l2_hit);
  l2_cache.next_level_memory = &memory;

  memory.clk(clk);
  memory.reset(reset);
  memory.address(address);
  memory.data_in(data_in);
  memory.we(we);
  memory.data_out(memory_data_out);

  SC_THREAD(forward_outputs);
  sensitive << l1_hit << l2_hit;
  dont_initialize();
}

void CacheSimulation::init(unsigned l1_cache_size, unsigned l2_cache_size,
                           unsigned cache_line_size, unsigned l1_latency,
                           unsigned l2_latency, unsigned memory_latency) {
  l1_cache.init(l1_cache_size, cache_line_size, l1_latency);
  l2_cache.init(l2_cache_size, cache_line_size, l2_latency);
}

void CacheSimulation::forward_outputs() {
  while (true) {
    wait();  // Wait for sensitivity list changes

    if (l1_hit.read()) {
      data_out.write(l1_data_out.read());
      hit.write(true);
    } else if (l2_hit.read()) {
      data_out.write(l2_data_out.read());
      hit.write(true);
    } else {
      hit.write(false);
      data_out.write(memory_data_out.read());
    }
    wait(2);
  }
}
