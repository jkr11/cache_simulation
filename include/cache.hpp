#ifndef CACHE_HPP
#define CACHE_HPP

#include <systemc.h>

#include "../include/types.h"
#include "../include/util.h"

class Cache : public sc_module {
 public:
  sc_in<bool> clk;
  sc_in<bool> reset;
  sc_in<uint32_t> address;
  sc_in<uint32_t> data_in;
  sc_in<bool> we;
  sc_out<uint32_t> data_out;
  sc_out<bool> hit;

  SC_HAS_PROCESS(Cache);

  Cache(sc_module_name name);
  ~Cache();

  void init(int size, int line_size, int latency);
  bool access(uint32_t address, uint32_t& out_data, uint32_t in_data,
              bool is_write);

 private:
  int size;
  int line_size;
  int latency;
  uint32_t* tags;
  uint32_t* data;
  int* lru;

  void update_lru(int acc_index);
  void cache_access();
};

class CacheSimulation : public sc_module {
 public:
  sc_in<bool> clk;
  sc_in<bool> reset;
  sc_in<uint32_t> address;
  sc_in<uint32_t> data_in;
  sc_in<bool> we;
  sc_out<uint32_t> data_out;
  sc_out<bool> hit;

  SC_HAS_PROCESS(CacheSimulation);

  CacheSimulation(sc_module_name name);
  void init(unsigned l1_cache_size, unsigned l2_cache_size,
            unsigned cache_line_size);

 private:
  Cache l1_cache;
  Cache l2_cache;

  sc_signal<uint32_t> l1_data_out;
  sc_signal<bool> l1_hit;
  sc_signal<uint32_t> l2_data_out;
  sc_signal<bool> l2_hit;

  void forward_outputs();
};

#endif  // CACHE_HPP