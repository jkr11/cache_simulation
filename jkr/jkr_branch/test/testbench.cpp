#include <systemc.h>

#include "../include/csv_parser.h"
#include "../include/types.h"

SC_MODULE(Testbench) {
  sc_clock clk;
  sc_signal<bool> start;
  sc_signal<bool> reset;
  sc_signal<bool> done;

  SC_CTOR(Testbench) : clk("clk", sc_time(10, SC_NS)) { SC_METHOD(update); }

  void update() {
    size_t num_requests;
    Request *requests = parse_csv("example.csv", &num_requests);
  }
};

int sc_main() {
  Testbench tb("Testbench");
  sc_start();
  return 0;
}