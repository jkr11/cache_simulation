#include "../include/memory.hpp"

Memory::Memory(sc_module_name name, int size) : sc_module(name), size(size) {
  memory = new uint32_t[size];

  SC_METHOD(memory_access);
  sensitive << clk.pos();
  dont_initialize();
}

Memory::~Memory() { delete[] memory; }

void Memory::memory_access() {
  if (reset.read()) {
    for (int i = 0; i < size; ++i) {
      memory[i] = 0;
    }
  } else {
    uint32_t addr = address.read() / 4;  //? 4-byte word addressing
    if (addr < size) {
      if (we.read()) {
        memory[addr] = data_in.read();
      }
      data_out.write(memory[addr]);
    }
  }
}