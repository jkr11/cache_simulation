#include "../include/simulation.h"
#include "../include/util.h"
#include "../include/types.h"
#include "memory.cpp"

Result run_simulation(int cycles, size_t numRequests, struct Request requests[]) {
  sc_signal<bool> clk;

  sc_signal<bool> writeThrough;

  sc_signal<bool> request;
  sc_signal<sc_bv<32>> inputData;
  sc_signal<sc_bv<32>> address;
  sc_signal<bool> rw;
  sc_signal<bool> ready;
  sc_signal<sc_bv<32>> dataFrom;

  MEMORY memory("mem",0);

  memory.clk(clk);
  memory.requestIncoming(request);
  memory.rw(rw);
  memory.addr(address);
  memory.rData(inputData);
  memory.wData(dataFrom);
  memory.ready(ready);

  int indexForInput = 0;

  clk.write(true);
  sc_start(1,SC_NS);
  clk.write(false);
  sc_start(1,SC_NS);

  bool lastR = false;
  int i;
  for(i = 0; i < cycles ; i++){
    if((size_t)indexForInput == numRequests){
      if(ready.read()){ // wait until all module finish their current operation
        if(lastR){// store the last read Data back to request
          requests[indexForInput-1].data = dataFrom.read().to_int();
        }
        break;
      }
    }
    if(ready.read()&&(size_t)indexForInput<numRequests){ // if l1 is ready for operation
      if(lastR){// store the read Data back to request
        requests[indexForInput-1].data = dataFrom.read().to_int();
      }
      request.write(true);
      sc_bv<32> addr = requests[indexForInput].addr;
      address.write(addr);
      sc_bv<32> inData = requests[indexForInput].data;
      inputData.write(inData);
      if(requests[indexForInput].we == 0){
        rw.write(false);
        lastR = true;
      }else{
        rw.write(true);
        lastR = false;
      }
      indexForInput++;
    }
    clk.write(true);
    sc_start(1,SC_NS);
    request.write(false);
    clk.write(false);
    sc_start(1,SC_NS);
  }
  sc_stop();


    return {(size_t)0,(size_t)(0),(size_t)(0),0};
}
int sc_main(int argc, char *argv[]){
  (void) argc;
  (void) argv;
  return 0;
};