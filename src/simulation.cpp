#include "../include/simulation.h"
#include "../include/util.h"
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
  CACHEL2 l2Cache("l2",l2CacheLatency,l2CacheLines,cacheLineSize,l1CacheLines);
  CACHEL1 l1Cache("l1",l1CacheLatency,l1CacheLines,cacheLineSize);

  l1Cache.l2 = l2Cache.internal;
  l2Cache.l1 = l1Cache.internal;


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
  sc_trace_file *wf;
  if(tracefile!=NULL){
    wf = sc_create_vcd_trace_file(tracefile);
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
  }

  int indexForInput = 0;
  bool allDone = false;

  // one tick for initialization
  clk.write(true);
  sc_start(1,SC_NS);
  clk.write(false);
  sc_start(1,SC_NS);

  bool lastR = false;
  int i;
  for(i = 0; i < cycles ; i++){
    if((size_t)indexForInput == numRequests){
      if(readyFromL1.read()&&readyFromL2ToL1.read()&&readyFromMemToL2.read()){ // wait until all module finish their current operation
        if(lastR){// store the last read Data back to request
          requests[indexForInput-1].data = dataFromL1.read().to_int();
        }
        allDone = true;
        break;
      }
    }
    if(readyFromL1.read()&&readyFromL2ToL1.read()&&readyFromMemToL2.read()&&(size_t)indexForInput<numRequests){ // if l1 is ready for operation
      if(lastR){// store the read Data back to request
        requests[indexForInput-1].data = dataFromL1.read().to_int();
      }
      requestToL1.write(true);
      sc_bv<32> addr = requests[indexForInput].addr;
      addressToL1.write(addr);
      sc_bv<32> inData = requests[indexForInput].data;
      inputDataToL1.write(inData);
      if(requests[indexForInput].we == 0){
        rwToL1.write(false);
        lastR = true;
      }else{
        rwToL1.write(true);
        lastR = false;
      }
      indexForInput++;
    }
    clk.write(true);
    sc_start(1,SC_NS);
    requestToL1.write(false);
    clk.write(false);
    sc_start(1,SC_NS);
  }
  sc_stop();
  if(tracefile!=NULL){
    sc_close_vcd_trace_file(wf);
  }



  if(allDone){
    return {(size_t)i,(size_t)(l1Cache.miss+l2Cache.miss),(size_t)(l1Cache.hits+l2Cache.hits),0};
  }
  else{
    return {SIZE_MAX, (size_t)(l1Cache.miss+l2Cache.miss), (size_t)(l1Cache.hits+l2Cache.hits), 0};
  }
}
