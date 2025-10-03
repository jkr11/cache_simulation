#ifndef MEMORY_CPP
#define MEMORY_CPP

#include <systemc.h>
#include <systemc>
#include <unordered_map>

// #define MEMLOG
// #define MEMLOG_TIME
SC_MODULE(MEMORY final) {
  int latency;
  sc_in<bool> clk;

  sc_in<bool> requestIncoming;
  sc_in<bool> rw;
  sc_in<sc_bv<32>> addr;
  sc_in<sc_bv<32>> rData; // the incomming data from L2

  sc_out<bool> ready;
  sc_out<sc_bv<32>> wData; // output data to L2

  std::unordered_map<uint32_t, uint8_t> internal;
  // this unordered_map allows the small range of simulation of 4GB memory in PC

  SC_CTOR(MEMORY);

  MEMORY(const sc_module_name &name, const int latency) : sc_module(name) {
    this->latency = latency;
    SC_THREAD(run);
    sensitive << clk.pos() << requestIncoming;
  }

  /**
   * @brief this Method is the entry point of Memory, it listens to the clock
   * and requestIncomming then perfroms the required function the latency will
   * be counted at the beginning of the request processing.
   */
  [[noreturn]] void run() {
    while (true) {
      wait();
      if (requestIncoming.read()) {
        ready.write(false);
        wait(2 * latency, SC_NS);

        const uint32_t address = addr.read().to_uint();
        if (rw.read()) { // true for write

          for (int i = 0; i < 4; i++) {
            internal[address + i] =
                rData.read().range((i + 1) * 8 - 1, i * 8).to_uint();
          }
        } else { // false for read
          sc_bv<32> data;
          for (int i = 0; i < 4; i++) {
            if (internal.find(address + i) == internal.end()) {
              data.range((i + 1) * 8 - 1, i * 8) = 0x00;
            } else {
              data.range((i + 1) * 8 - 1, i * 8) = internal[address + i];
            }
          }
          wData.write(data);
        }
        ready.write(true);
      } else {
        ready.write(true);
      }
    }
  }

  ~MEMORY() override = default;
};

#endif
