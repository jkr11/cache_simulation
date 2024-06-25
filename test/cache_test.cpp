#include <systemc.h>

SC_MODULE(CacheModule) {
  sc_in<uint32_t> addr;
  sc_in<uint32_t> write_in;
  sc_out<bool> hit;

  SC_CTOR(CacheModule) {
    SC_THREAD(cache_access);
    sensitive << addr;
  }

  void cache_access() {
    while (true) {
      wait();

      if (addr.read() == 0x10) {
        hit.write(true);
        cout << "Hit! Address: 0x10" << endl;
      } else {
        hit.write(false);
        cout << "Miss! Address: " << hex << addr.read() << endl;
      }
    }
  }
};

int sc_main(int argc, char* argv[]) {
  sc_signal<uint32_t> addr;
  sc_signal<uint32_t> write_in;
  sc_signal<bool> hit;

  CacheModule cache("cache");
  cache.addr(addr);
  cache.write_in(write_in);
  cache.hit(hit);

  addr = 0x10;       
  write_in = 0xABCD;  

  sc_start();

  return 0;
}
