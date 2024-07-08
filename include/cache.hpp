#ifndef CACHE_HPP
#define CACHE_HPP

#include <systemc.h>

#include "../include/memory_interface.hpp"
#include "../include/types.h"
#include "../include/util.h"
#include "memory.hpp"

class Cache : public MemoryInterface {
 public:
  sc_in<bool> clk;
  sc_in<bool> reset;
  sc_in<bool> we;
  sc_in<uint32_t> address;
  sc_in<uint32_t> data_in;
  sc_out<uint32_t> data_out;
  sc_out<bool> hit;

  SC_HAS_PROCESS(Cache);

  Cache(sc_module_name name);
  ~Cache();

  void init(int size, int line_size, int latency);
  bool access(uint32_t address, uint32_t& out_data, uint32_t in_data,
              bool is_write) override;

  MemoryInterface* next_level_memory;

 private:
  void cache_access();

  int size;
  int line_size;
  int latency;

  uint32_t* tags;
  uint8_t* data;  // byte-addressed
};

class CacheSimulation : public sc_module {
 public:
  sc_in<bool> clk;
  sc_in<bool> reset;
  sc_in<bool> we;
  sc_in<uint32_t> address;
  sc_in<uint32_t> data_in;
  sc_out<uint32_t> data_out;
  sc_out<bool> hit;

  SC_HAS_PROCESS(CacheSimulation);

  CacheSimulation(sc_module_name name);

  void init(unsigned l1_cache_size, unsigned l2_cache_size,
            unsigned cache_line_size, unsigned l1_latency, unsigned l2_latency,
            unsigned memory_latency);

 private:
  void forward_outputs();

  Cache l1_cache;
  Cache l2_cache;
  Memory memory;

  sc_signal<uint32_t> l1_data_out;
  sc_signal<bool> l1_hit;
  sc_signal<uint32_t> l2_data_out;
  sc_signal<bool> l2_hit;
  sc_signal<uint32_t> memory_data_out;
};

#endif  // CACHE_HPP