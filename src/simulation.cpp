#include "../include/simulation.h"

#include "../include/types.h"
#include "cacheL1.cpp"
#include "cacheL2.cpp"
#include "memory.cpp"

Result run_simulation(int cycles, unsigned l1CacheLines, unsigned l2CacheLines,
                      unsigned cacheLineSize, unsigned l1CacheLatency,
                      unsigned l2CacheLatency, unsigned memoryLatency,
                      size_t numRequests, struct Request requests[],
                      const char *tracefile) {
  sc_signal<bool> clk;

  sc_signal<bool> writeThrough;

  sc_signal<bool> requestToL1;
  sc_signal<sc_bv<32>> inputDataToL1;
  sc_signal<sc_bv<32>> addressToL1;
  sc_signal<bool> rwToL1;
  sc_signal<bool> readyFromL1;
  sc_signal<sc_bv<32>> dataFromL1;

  sc_signal<bool> readyFromL2ToL1;
  sc_signal<sc_bv<32>> dataFromL2ToL1;

  sc_signal<bool> requestFromL1ToL2;
  sc_signal<sc_bv<32>> dataFromL1ToL2;
  sc_signal<sc_bv<32>> addressFromL1ToL2;
  sc_signal<bool> rwFromL1ToL2;

  sc_signal<bool> readyFromMemToL2;
  sc_signal<sc_bv<32>> dataFromMemToL2;

  sc_signal<bool> requestFromL2ToMem;
  sc_signal<bool> rwFromL2ToMem;
  sc_signal<sc_bv<32>> addressFromL2ToMem;
  sc_signal<sc_bv<32>> dataFromL2ToMem;
  
  MEMORY memory("mem",memoryLatency);
  CACHEL2 l2Cache("l2",l2CacheLatency,l2CacheLines,cacheLineSize);
  CACHEL1 l1Cache("l1",l1CacheLatency,l1CacheLines,cacheLineSize);


  l1Cache.clk(clk);
  l1Cache.requestIncoming(requestToL1);
  l1Cache.inputData(inputDataToL1);
  l1Cache.address(addressToL1);
  l1Cache.rw(rwToL1);
  l1Cache.readyFromLastStage(readyFromL2ToL1);
  l1Cache.dataFromLastStage(dataFromL2ToL1);
  l1Cache.requestToLastStage(requestFromL1ToL2);
  l1Cache.rwToLastStage(rwFromL1ToL2);
  l1Cache.addressToLastStage(addressFromL1ToL2);
  l1Cache.outputToLastStage(dataFromL1ToL2);
  l1Cache.ready(readyFromL1);
  l1Cache.outputData(dataFromL1);
  l1Cache.isWriteThrough(writeThrough);


  l2Cache.clk(clk);
  l2Cache.requestIncoming(requestFromL1ToL2);
  l2Cache.inputData(dataFromL1ToL2);
  l2Cache.address(addressFromL1ToL2);
  l2Cache.rw(rwFromL1ToL2);
  l2Cache.readyFromLastStage(readyFromMemToL2);
  l2Cache.dataFromLastStage(dataFromMemToL2);
  l2Cache.requestToLastStage(requestFromL2ToMem);
  l2Cache.rwToLastStage(rwFromL2ToMem);
  l2Cache.addressToLastStage(addressFromL2ToMem);
  l2Cache.outputToLastStage(dataFromL2ToMem);
  l2Cache.ready(readyFromL2ToL1);
  l2Cache.outputData(dataFromL2ToL1);
  l2Cache.isWriteThrough(writeThrough);

  memory.clk(clk);
  memory.requestIncoming(requestFromL2ToMem);
  memory.rw(rwFromL2ToMem);
  memory.addr(addressFromL2ToMem);
  memory.rData(dataFromL2ToMem);
  memory.wData(dataFromMemToL2);
  memory.ready(readyFromMemToL2);
  
  sc_trace_file *wf = sc_create_vcd_trace_file("debug");
  wf->set_time_unit(1,SC_NS);

  sc_trace(wf, clk,"clock");
  
  sc_trace(wf, requestToL1, "requestToL1");
  sc_trace(wf, inputDataToL1, "inputDataToL1");
  sc_trace(wf, addressToL1, "addressToL1");
  sc_trace(wf, rwToL1, "rwToL1");
  sc_trace(wf, readyFromL1, "readyFromL1");
  sc_trace(wf, dataFromL1, "dataFromL1");

  sc_trace(wf, readyFromL2ToL1, "readyFromL2ToL1");
  sc_trace(wf, dataFromL2ToL1, "dataFromL2ToL1");

  sc_trace(wf, requestFromL1ToL2, "requestFromL1ToL2");
  sc_trace(wf, dataFromL1ToL2, "dataFromL1ToL2");
  sc_trace(wf, addressFromL1ToL2, "addressFromL1ToL2");
  sc_trace(wf, rwFromL1ToL2, "rwFromL1ToL2");

  sc_trace(wf, readyFromMemToL2, "readyFromMemToL2");
  sc_trace(wf, dataFromMemToL2, "dataFromMemToL2");

  sc_trace(wf, requestFromL2ToMem, "requestFromL2ToMem");
  sc_trace(wf, rwFromL2ToMem, "rwFromL2ToMem");
  sc_trace(wf, addressFromL2ToMem, "addressFromL2ToMem");
  sc_trace(wf, dataFromL2ToMem, "dataFromL2ToMem");

  int indexForInput = 0;
  bool allDone = false;

  int i;
  for(i = 0; i < cycles ; i++){
    clk.write(true);
    sc_start(1,SC_NS);
    
    if(readyFromL1.read()){
      requestToL1.write(true);
      sc_bv<32> addr = requests[indexForInput].addr;
      addressToL1.write(addr);
      sc_bv<32> inData = requests[indexForInput].data;
      inputDataToL1.write(inData);
      if(requests[indexForInput].we == 0){
        rwToL1.write(false);
      }else{
        rwToL1.write(true);
      }
      if(indexForInput == numRequests){
        allDone = true;
        break;
      }
      indexForInput++;
    }
    clk.write(false);
    sc_start(1,SC_NS);
    requestToL1.write(false);
  }
  sc_stop();
  sc_close_vcd_trace_file(wf);
  if(allDone){
    return {i,l1Cache.miss+l2Cache.miss,l1Cache.hits+l2Cache.hits,0};
  }
  else{
    return {SIZE_MAX, l1Cache.miss+l2Cache.miss, l1Cache.hits+l2Cache.hits, 0};
  }
  
}