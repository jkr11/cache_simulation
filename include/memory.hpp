#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <systemc.h>

class Memory : public sc_module {
 public:
  sc_in<bool> clk;
  sc_in<bool> reset;
  sc_in<bool> we;
  sc_in<uint32_t> address;
  sc_in<uint32_t> data_in;
  sc_out<uint32_t> data_out;

  SC_HAS_PROCESS(Memory);

  Memory(sc_module_name name, int size);

  ~Memory();

 private:
  uint32_t* memory;
  int size;

  void memory_access();
};

#endif  // MEMORY_HPP