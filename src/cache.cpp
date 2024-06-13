#include <systemc>

SC_MODULE(CACHE){
    int latency;
    int cacheLines;
    // here we need to consider when we have a larger cache line as our memory block, 
    // do we have also to implement a logic that we write the dirty data multiple times back to memory
    // coz I have read from the data structure of "Request" that out memory block might just have 32 bit
    int cacheLineSize; 

    int miss;
    int hits;

    sc_input<sc_bv<cacheLineSize*8>> inputData;
    sc_input<sc_bv<32>> address;
    sc_input<bool> rw;  

    sc_output<bool> rwToLastStage; // here can "last stage" be memory or l2-cache
    sc_output<sc_vector<cacheLineSize*8>> outputData;

    // we can possibly divide these bits into : Control bits|Tag bits|Data bits
    sc_bv<cacheLineSize*8> internal[cacheLines]; 
}

