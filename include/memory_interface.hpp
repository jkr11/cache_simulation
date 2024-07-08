#ifndef MEMORY_INTERFACE_HPP
#define MEMORY_INTERFACE_HPP

#include <systemc.h>

class MemoryInterface : public sc_module {
 public:
  SC_HAS_PROCESS(MemoryInterface);

  MemoryInterface(sc_module_name name) : sc_module(name) {}
  virtual bool access(uint32_t address, uint32_t& out_data, uint32_t in_data,
                      bool is_write) = 0;
};

#endif  // MEMORY_INTERFACE_HPP
