#include <systemc.h>

#include "types.h"

SC_MODULE(Cache) {
  sc_in<bool> clk;
  sc_in<bool> req_read;
  sc_in<bool> req_write;
  sc_in<uint32_t> req_addr;
  sc_in<uint32_t> req_data;

  sc_out<uint32_t> read_data;
  sc_out<bool> hit;
  sc_out<bool> done;

  int cachesize;
  sc_uint<32>* cache;
  sc_uint<32>* tags;
  sc_signal<bool>* valid;

  SC_CTOR(Cache) {
    SC_METHOD(update);
    sensitive << clk.pos();
  }

  void init(size_t size) {
    cachesize = size;
    cache = new sc_uint<32>[cachesize];
    tags = new sc_uint<32>[cachesize];
    valid = new sc_signal<bool>[cachesize];
    for(int i = 0; i < cachesize; i++) {
      valid[i].write(false);
    }
        
  }

  void update() {
    if(req_read.read()) {
      sc_uint<32> index = req_addr.read() % cachesize;
      sc_uint<32> tag = req_addr.read() / cachesize;

      if(valid[index].read() && tags[index] == tag) {
        // This means we have a cache hit
        hit.write(true);
        read_data.write(cache[index]);
        done.write(true);
      } else {
        hit.write(false);
        done.write(true);
      }
    } else if (!req_read.read()) {
      sc_uint<32> index = req_addr.read() % cachesize;
      sc_uint<32> tag = req_addr.read() / cachesize;
      cache[index] = req_data.read();
      tags[index] = tag;
      valid[index] = true;
    } else {
      done.write(false);
    }
  }

  ~Cache() {
    delete[] cache;
    delete[] tags;
    delete[] valid;
  }

};
