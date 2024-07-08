#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <unordered_map>

#include "memory_interface.hpp"

class Memory : public MemoryInterface {
 public:
  sc_in<bool> clk;
  sc_in<bool> reset;
  sc_in<bool> we;
  sc_in<uint32_t> address;
  sc_in<uint32_t> data_in;
  sc_out<uint32_t> data_out;

  SC_HAS_PROCESS(Memory);

  Memory(sc_module_name name);
  ~Memory();

  bool access(uint32_t address, uint32_t& out_data, uint32_t in_data,
              bool is_write) override;

 private:
  void memory_access();
  std::unordered_map<uint32_t, uint32_t> memory;
};

#endif  // MEMORY_HPP