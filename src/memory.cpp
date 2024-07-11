#include "../include/memory.hpp"

#include <systemc.h>

#include <iostream>

Memory::Memory(sc_module_name name) : MemoryInterface(name) {
  SC_THREAD(memory_access);
  sensitive << clk.pos();
  dont_initialize();
}

Memory::~Memory() {}

bool Memory::access(uint32_t address, uint32_t& out_data, uint32_t in_data,
                    bool is_write) {
  if (is_write) {
    memory[address] = in_data;
    std::cout << "Memory Write Address: 0x" << std::hex << address
              << " Data: " << std::dec << memory[address] << std::endl;
  } else {
    out_data = memory[address];
    std::cout << "Memory Read Address: 0x" << std::hex << address
              << " Data: " << std::dec << memory[address] << std::endl;
  }
  return true;
}

void Memory::memory_access() {
  while (true) {
    wait();
    if (reset.read()) {
      memory.clear();
    } else {
      uint32_t addr = address.read();
      if (we.read()) {
        memory[addr] = data_in.read();
        std::cout << "Memory Write Address: 0x" << std::hex << addr
                  << " Data: " << std::dec << memory[addr] << std::endl;
      } else {
        data_out.write(memory[addr]);
        std::cout << "Memory Read Address: 0x" << std::hex << addr
                  << " Data: " << std::dec << memory[addr] << std::endl;
      }
    }
  }
}