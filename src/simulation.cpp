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
    if(readyFromL1.read()&&readyFromL2ToL1.read()&&readyFromMemToL2.read()&&(size_t)indexForInput<numRequests){ // if all modules are ready for operation
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
    clk.write(true); // clock running
    sc_start(1,SC_NS);
    requestToL1.write(false);
    clk.write(false);
    sc_start(1,SC_NS);
  }
  sc_stop();
  if(tracefile!=NULL){
    sc_close_vcd_trace_file(wf);
  }
  /* Gatter Calculation:
    L1:
      Speicherung: (Tagbits + valid + CachelineSize)*Cachlines

      Tag Comparator: : TagLength + TagLength -1 (XOR und OR) + 1 (AND)

      CachelineSize*8-bit-2-to-1 MUX : CachelineSize*8 * 4 
      
      Control Unit:
      2-to-1 MUX f ̈ur Tag: TagLength * 4
      2-to-1 Mux f ̈ur Index: IndexLength * 4

      FSM:
        Zustandspeicherung: log2(13) = 4 (bits) D-Flip-Flop, Gatter = 4 * 5 = 20
        Zustand ̈ubergang: (5 Eingabe + 4 bits) * 4 bits = 36
        Ausgabe: 4 bits * 7 Ausgabe = 28

      Address Calculator:
        32-bit Adder : (2 And + 1 Or + 2 XOR) * 32 = 160
        offset Adder : (2 And + 1 Or + 2 XOR) * offsetLength
        32-bit-2-to-1 MUX: 32 * 4 = 128
      Data Calculator:
        Speicherung: (2*CachelineSize*8) D-Latch(jeweils 5 Gatter)
        1-to-2 Decoder: 1 Not
        2 * 1-bit-2-to-1 MUX: 2 * 4
        CachelineSize-bit-2-to-1 MUX: CachelineSize*4
        CachelineSize * 32-bit-2-to-1 MUX : CachelineSize*32*4
        CachelineSize * 32-bit-2-to-1 DEMUX : CachelineSize*32*3
    L2:
      Speicherung: (Tagbits + valid + CachelineSize)*Cachlines

      Tag Comparator: : TagLength + TagLength -1 (XOR und OR) + 1 (AND)

      Control Unit:
        2-to-1 MUX f ̈ur Tag: TagLength * 4
        2-to-1 MUX f ̈ur Index: IndexLength * 4
        2-to-1 MUX f ̈ur Offset : OffsetLength * 4
      FSM:
        Zustandspeicherung: log2(8) = 3 (bits) D-Flip-Flop, Gatter = 3 * 5 = 15
        Zustand ̈ubergang: (6 Eingabe + 3 bits) * 3 bits = 27
        Ausgabe: 3 bits * 7 Ausgabe = 21
      Address Counter:
        32-bit-2-to-1 MUX : 32 * 4 = 128
        32-bit D-Flip-Flop = 32 * 5 = 160
        32-bit Adder = 160
        32-bit Comparator = 32 XOR + 31 OR = 63
        1 Inverter = 1
      Address Calculator:
        gleich wie bei L1
      Data Calculator:
        Speicherung: (2*CachelineSize*8) D-Latch(jeweils 5 Gatter)
        1-to-2 Decoder: 1 Not
        2 * 1-bit-2-to-1 MUX: 2 * 4
        CachelineSize-bit-2-to-1 MUX: CachelineSize*4
        CachelineSize * 32-bit-2-to-1 MUX : CachelineSize*32*4
        Kein Demux n ̈otig, da wir kein 32-bit Daten auslesen sollen



  
  */
  int GatterCount = 0;
  int offsetLength = (int)(log(cacheLineSize)/log(2));
  int L1indexLength = (int)(log(l1CacheLines)/log(2));
  int L1tagbits = 32 - offsetLength-L1indexLength;
  int L2indexLength = (int)(log(l2CacheLines)/log(2));
  int L2tagbits = 32 - offsetLength-L2indexLength;
  GatterCount += (L1tagbits+1+cacheLineSize*8)*l1CacheLines + 2*L1tagbits + (cacheLineSize*8)*4 + L1tagbits*4 + L1indexLength*4 + 20 + 26 + 28;
  GatterCount += 160 + 5*offsetLength + 128 + 16*cacheLineSize*5+1 + 8 + cacheLineSize*8 * 4 + cacheLineSize*32*7;
  GatterCount += (L2tagbits+1+cacheLineSize*8)*l2CacheLines + 32*4 + 15 +27 +21+128+160+160+64 +160 + 5*offsetLength + 128;
  GatterCount += 16*cacheLineSize*5+1 + 8 + cacheLineSize*8 * 4 + cacheLineSize*32*4;

  if(allDone){
    return {(size_t)i,(size_t)(l1Cache.miss+l2Cache.miss),(size_t)(l1Cache.hits+l2Cache.hits),(size_t)GatterCount};
  }
  else{
    return {SIZE_MAX, (size_t)(l1Cache.miss+l2Cache.miss), (size_t)(l1Cache.hits+l2Cache.hits), (size_t)GatterCount};
  }
}

int sc_main(int argc, char* argv[]){
  (void) argc;
  (void) argv;
  return 0;
}
