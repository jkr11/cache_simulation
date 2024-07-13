#ifndef CACEHL2_CPP
#define CACEHL2_CPP

#include <systemc.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <systemc>

#include "cacheL1.cpp"
#include "types.h"

SC_MODULE(CACHEL2) {
  int latency;
  int cacheLines;  // this has also to be 2er Potenz

  // here i took maximal cacheLineSize as 128 bit, which is also the largest
  // range in the industry，so the max of this Size is 16 Byte
  int cacheLineSize;

  int miss;
  int hits;

  CacheLine* l1;  // the internal storage of l1

  // tag|index|Offset
  int offsetLength;
  int indexOffset;  // the index position for a 32 bit address
  int indexLength;
  int tagOffset;
  int tagbits;  // the length of tagbits

  int waitingForLatency = 0;

  bool isWriteT;

  const char* name;
  sc_in<bool> isWriteThrough;  // if this is a write through request, the hits
                               // will not be increased
  sc_in<bool> clk;
  sc_in<bool> requestIncoming;
  sc_in<sc_bv<32>> inputData;
  sc_in<sc_bv<32>> address;
  sc_in<bool> rw;

  // communication with last stage
  sc_in<bool> readyFromLastStage;
  sc_in<sc_bv<32>> dataFromLastStage;  // this will give the current index of
                                       // the internal storage of l1
  sc_out<bool> requestToLastStage;
  sc_out<bool> rwToLastStage;
  sc_out<sc_bv<32>> addressToLastStage;
  sc_out<sc_bv<32>> outputToLastStage;  // output forward to inside

  sc_out<bool> ready;            // this component is ready to use
  sc_out<sc_bv<32>> outputData;  // output to outside

  // the internal storage
  CacheLine* internal;

  SC_CTOR(CACHEL2);
  CACHEL2(sc_module_name name, int latency, int cacheLines, int cacheLineSize)
      : sc_module(name) {
    this->latency = latency;
    this->cacheLines = cacheLines;
    this->cacheLineSize = cacheLineSize;
    waitingForLatency = 0;
    hits = 0;
    miss = 0;
    offsetLength = (int)(log(cacheLineSize) / log(2));
    indexLength = (int)(log(cacheLines) / log(2));
    tagbits = 32 - offsetLength - indexLength;
    tagOffset = 32 - tagbits;

    indexOffset = 32 - tagbits - indexLength;

    internal = new CacheLine[cacheLines];
    for (int i = 0; i < cacheLines; i++) {
      internal[i].bytes = (uint8_t*)calloc(cacheLineSize, sizeof(uint8_t));
      ;
      internal[i].empty = 1;
    }

    this->name = (const char*)name;
    SC_THREAD(run);
    sensitive << clk.pos() << requestIncoming;
  }
  // as required the cache is full associative and shall be replaced with LRU
  void run() {
    while (true) {
      wait();

      if (requestIncoming
              .read()) {  // upon request shall the component start to work
        std::cout << name << " received request:" << requestIncoming.read()
                  << std::endl;
        isWriteT = isWriteThrough.read();
        ready.write(false);
        wait(latency * 2, SC_NS);
        /*
        while(waitingForLatency<latency){ // latency counter
            ready.write(false);
            wait();
            //std::cout<<name<<" waiting for latency"<< std::endl;
            waitingForLatency++;
        }
        */
        if (rw.read()) {  // write enable
          int receivedData = 0;
          for (int i = 0; i < cacheLineSize; i++) {
            receivedData += l1[inputData.read().to_int()].bytes[i];
          }
          std::cout << name << " received <write> with addr:" << std::hex
                    << std::setw(8) << std::setfill('0')
                    << address.read().to_int() << " and data:" << std::hex
                    << std::setw(8) << std::setfill('0') << receivedData
                    << std::endl;
          write();
        } else {
          std::cout << name << " received <read> with addr:" << std::hex
                    << std::setw(8) << std::setfill('0')
                    << address.read().to_int() << std::endl;
          read();
        }
<<<<<<< HEAD
      } else {
        // std::cout<<name<<" waiting for request"<< std::endl;
=======
    }

    int ifExist(int tag,int index){ // this method checks if the given address exist in the cache, returns the corresponding array index, and -1 if not exist
        if(internal[index%cacheLines].empty == 1){
            return -1;
        }
        int tag_tmp = internal[index%cacheLines].tag;
        if (tag_tmp == tag)
            return index;
        return -1;
    }

    void write(){
        bool hit = true;
        int index = -1;
        int t_tmp = address.read().range(31,tagOffset).to_uint();
        int i_tmp = address.read().range(tagOffset-1,indexOffset).to_uint();
        index = ifExist(t_tmp,i_tmp);
            if(index==-1){ // in the cache there are no such information
                std::cout<<name<<" miss by writing with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int()<< " detected, sending signal to next level"<< std::endl;
                miss++;
                hit = false;
                index = loadFromMem();
            }
            if(hit&&!isWriteT){
                std::cout<<name<<" hit with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int() << std::endl;
                hits++;
            }
            // at this point, the required data is successfully loaded from last stage
            //calculate the offset and change the required data
            for(int i = 0; i< cacheLineSize;i++){ // direct communication with l1
                internal[index%cacheLines].bytes[i] = l1[inputData.read().to_int()].bytes[i];
            }
            writeThrough(index%cacheLines);
            //wait(); //wait for lastStage to exicute and change the request Singnal to false
            requestToLastStage.write(false);
        	ready.write(true);
            std::cout<<name<<" finished with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int() << std::endl;
    }
    void read(){
        bool hit = true;
        int index = -1;
        int t_tmp = address.read().range(31,tagOffset).to_uint();
        int i_tmp = address.read().range(tagOffset-1,indexOffset).to_uint();
        index = ifExist(t_tmp,i_tmp);
        if(index==-1){ // in the cache there are no such information
            std::cout<<name<<" miss by reading with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int()<< " detected, sending signal to next level"<< std::endl;
            miss++;
            hit = false;
            index = loadFromMem();
        }
        if(hit){
            std::cout<<name<<" hit with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int() << std::endl;
            hits++;
        }
        outputData.write(index%cacheLines); // send the index of internal for l1 to gain immediate access to whole cache line
>>>>>>> Entscheidbarkeit
        ready.write(true);
      }
      waitingForLatency = 0;
    }
  }

<<<<<<< HEAD
  int ifExist(int tag,
              int index) {  // this method checks if the given address exist in
                            // the cache, returns the corresponding array index,
                            // and -1 if not exist
    if (internal[index % cacheLines].empty == 1) {
      return -1;
=======
    void writeThrough(int index){
        sc_bv<32> addressToMem = address.read();
        addressToMem.range(offsetLength-1,0) = 0;//address alignment
        printCacheLine(index%cacheLines);
        for(int i = 0; i< (int)(cacheLineSize/4);i++){
            while(!readyFromLastStage.read()){
                wait();
            }
            requestToLastStage.write(true);
            rwToLastStage.write(true);
            sc_bv<32> data_tmp;
            for(int j = i*4; j < (i+1)*4; j++){
                //std::cout<<name<<" write Through data loaded "<< std::hex << std::setw(2) << std::setfill('0')<<(uint32_t)internal[index].bytes[j] << std::endl;
                data_tmp.range((j-i*4+1)*8-1,(j-i*4)*8) = internal[index].bytes[j];
            }
            outputToLastStage.write(data_tmp);
            std::cout<<name<<" write Through data wired "<< std::hex << std::setw(2) << std::setfill('0')<<data_tmp.to_int() << std::endl;
            std::cout<<name<<" write Through data wired in binary "<<data_tmp.to_string() << std::endl;
            addressToLastStage.write(addressToMem);
            wait();
            requestToLastStage.write(false);
            wait(); // wait for the ready from mem to change
            addressToMem.range(31,0) = addressToMem.to_int()+4;
        }
>>>>>>> Entscheidbarkeit
    }
    int tag_tmp = internal[index % cacheLines].tag;
    if (tag_tmp == tag) return index;
    return -1;
  }

  void write() {
    bool hit = true;
    int index = -1;
    int t_tmp = address.read().range(31, tagOffset).to_uint();
    int i_tmp = address.read().range(tagOffset - 1, indexOffset).to_uint();
    index = ifExist(t_tmp, i_tmp);
    if (index == -1) {  // in the cache there are no such information
      std::cout << name << " miss by writing with addr: " << std::hex
                << std::setw(8) << std::setfill('0') << address.read().to_int()
                << " detected, sending signal to next level" << std::endl;
      miss++;
      hit = false;
      sc_bv<32> addressToMem;
      addressToMem.range(31, 0) = address.read().to_int();
      addressToMem.range(offsetLength - 1, 0) =
          0;  // align with the starting address with each line
      for (int i = 0; i < (int)(cacheLineSize / 4); i++) {
        while (!readyFromLastStage
                    .read()) {  // keep on waitin until last Stage is ready
          wait();
        }
        std::cout
            << name
            << " with last stage ready, start sending signal to last stage"
            << std::endl;
        requestToLastStage.write(true);
        rwToLastStage.write(false);

        std::cout << name << " aligned addr: " << std::hex << std::setw(8)
                  << std::setfill('0') << addressToMem.to_int() << std::endl;
        addressToLastStage.write(addressToMem);  // address alignment with 4

        std::cout << name << " sended addr: " << std::hex << std::setw(8)
                  << std::setfill('0') << address.read().to_int() << std::endl;
        wait();
        requestToLastStage.write(false);
        while (!readyFromLastStage
                    .read()) {  // keep on waitin until last Stage is ready
          wait();
        }
        std::cout << name << " with last stage ready for data preperation"
                  << std::endl;
        requestToLastStage.write(false);
        std::cout << name << " received data from last stage: " << std::hex
                  << std::setw(8) << std::setfill('0')
                  << dataFromLastStage.read().to_int() << std::endl;
        index = address.read().range(tagOffset - 1, indexOffset).to_uint();
        internal[index % cacheLines].tag =
            address.read().range(31, tagOffset).to_uint();
        for (int j = i * 4; j < (i + 1) * 4; j++) {
          internal[index % cacheLines].bytes[j] =
              dataFromLastStage.read()
                  .range((j - i * 4 + 1) * 8 - 1, (j - i * 4) * 8)
                  .to_uint();  // big indian
        }
        // increment of the address by 4
        addressToMem.range(31, 0) = addressToMem.to_int() + 4;
      }
      internal[index % cacheLines].empty = 0;
    }
<<<<<<< HEAD
    if (hit && !isWriteT) {
      std::cout << name << " hit with addr: " << std::hex << std::setw(8)
                << std::setfill('0') << address.read().to_int() << std::endl;
      hits++;
    }
    // at this point, the required data is successfully loaded from last stage
    // calculate the offset and change the required data
    for (int i = 0; i < cacheLineSize; i++) {  // direct communication with l1
      internal[index % cacheLines].bytes[i] =
          l1[inputData.read().to_int()].bytes[i];
    }
    writeThrough(index % cacheLines);
    // wait(); //wait for lastStage to exicute and change the request Singnal to
    // false
    requestToLastStage.write(false);
    ready.write(true);
    std::cout << name << " finished with addr: " << std::hex << std::setw(8)
              << std::setfill('0') << address.read().to_int() << std::endl;
  }
  void read() {
    bool hit = true;
    int index = -1;
    int t_tmp = address.read().range(31, tagOffset).to_uint();
    int i_tmp = address.read().range(tagOffset - 1, indexOffset).to_uint();
    index = ifExist(t_tmp, i_tmp);
    if (index == -1) {  // in the cache there are no such information
      std::cout << name << " miss by reading with addr: " << std::hex
                << std::setw(8) << std::setfill('0') << address.read().to_int()
                << " detected, sending signal to next level" << std::endl;
      miss++;
      hit = false;
      sc_bv<32> addressToMem;
      addressToMem.range(31, 0) = address.read().to_int();
      addressToMem.range(offsetLength - 1, 0) =
          0;  // align with the starting address with each line
      for (int i = 0; i < (int)(cacheLineSize / 4); i++) {
        while (!readyFromLastStage
                    .read()) {  // keep on waitin until last Stage is ready
          wait();
=======
    int loadFromMem(){
        sc_bv<32> addressToMem; 
        int index = address.read().range(tagOffset-1,indexOffset).to_uint();;
        addressToMem.range(31,0) = address.read().to_int();
        addressToMem.range(offsetLength-1,0) = 0; // align with the starting address with each line
        for(int i = 0; i< (int)(cacheLineSize/4);i++){
            while(!readyFromLastStage.read()){ // keep on waitin until last Stage is ready
                wait();
            }
            std::cout<<name<<" with last stage ready, start sending signal to last stage"<< std::endl;
            requestToLastStage.write(true);
            rwToLastStage.write(false);
            
            std::cout<<name<<" aligned addr: "<< std::hex << std::setw(8) << std::setfill('0')<< addressToMem.to_int()<< std::endl;
            addressToLastStage.write(addressToMem); // address alignment with 4

            std::cout<<name<<" sended addr: "<< std::hex << std::setw(8) << std::setfill('0')<< address.read().to_int()<< std::endl;
            wait();
            requestToLastStage.write(false);
            wait(); // wait for ready from mem to change
            while(!readyFromLastStage.read()){ // keep on waitin until last Stage is ready
                wait();
            }
            std::cout<<name<<" with last stage ready for data preperation"<< std::endl;
            requestToLastStage.write(false);
            std::cout<<name<<" received data from last stage: "<< std::hex << std::setw(8) << std::setfill('0')<< dataFromLastStage.read().to_int()<< std::endl;
            internal[index%cacheLines].tag = address.read().range(31,tagOffset).to_uint();
            for(int j = i*4; j<(i+1)*4;j++){
                internal[index%cacheLines].bytes[j] = dataFromLastStage.read().range((j-i*4+1)*8-1,(j-i*4)*8).to_uint();
            }
            internal[index%cacheLines].empty = 0;
            // increment of the address by 4
            addressToMem.range(31,0) = addressToMem.to_int()+4;
        }
        internal[index%cacheLines].empty = 0;
        return index;
    }
    ~CACHEL2(){
        for(int i = 0; i< cacheLines; i++){
            free(internal[i].bytes);
>>>>>>> Entscheidbarkeit
        }
        std::cout
            << name
            << " with last stage ready, start sending signal to last stage"
            << std::endl;
        requestToLastStage.write(true);
        rwToLastStage.write(false);

        std::cout << name << " aligned addr: " << std::hex << std::setw(8)
                  << std::setfill('0') << addressToMem.to_int() << std::endl;
        addressToLastStage.write(addressToMem);  // address alignment with 4

        std::cout << name << " sended addr: " << std::hex << std::setw(8)
                  << std::setfill('0') << address.read().to_int() << std::endl;
        wait();
        requestToLastStage.write(false);
        while (!readyFromLastStage
                    .read()) {  // keep on waitin until last Stage is ready
          wait();
        }
        std::cout << name << " with last stage ready for data preperation"
                  << std::endl;
        requestToLastStage.write(false);
        std::cout << name << " received data from last stage: " << std::hex
                  << std::setw(8) << std::setfill('0')
                  << dataFromLastStage.read().to_int() << std::endl;
        index = address.read().range(tagOffset - 1, indexOffset).to_uint();
        internal[index % cacheLines].tag =
            address.read().range(31, tagOffset).to_uint();
        for (int j = i * 4; j < (i + 1) * 4; j++) {
          internal[index % cacheLines].bytes[j] =
              dataFromLastStage.read()
                  .range((j - i * 4 + 1) * 8 - 1, (j - i * 4) * 8)
                  .to_uint();
        }
        internal[index % cacheLines].empty = 0;
        // increment of the address by 4
        addressToMem.range(31, 0) = addressToMem.to_int() + 4;
      }
    }
    if (hit) {
      std::cout << name << " hit with addr: " << std::hex << std::setw(8)
                << std::setfill('0') << address.read().to_int() << std::endl;
      hits++;
    }
    outputData.write(index %
                     cacheLines);  // send the index of internal for l1 to gain
                                   // immediate access to whole cache line
    ready.write(true);
    std::cout << name << " finished with addr: " << std::hex << std::setw(8)
              << std::setfill('0') << address.read().to_int() << std::endl;
  }

  void writeThrough(int index) {
    sc_bv<32> addressToMem = address.read();
    addressToMem.range(offsetLength - 1, 0) = 0;  // address alignment
    printCacheLine(index % cacheLines);
    for (int i = 0; i < (int)(cacheLineSize / 4); i++) {
      while (!readyFromLastStage.read()) {
        wait();
      }
      requestToLastStage.write(true);
      rwToLastStage.write(true);
      sc_bv<32> data_tmp;
      for (int j = i * 4; j < (i + 1) * 4; j++) {
        // std::cout<<name<<" write Through data loaded "<< std::hex <<
        // std::setw(2) << std::setfill('0')<<(uint32_t)internal[index].bytes[j]
        // << std::endl;
        data_tmp.range((j - i * 4 + 1) * 8 - 1, (j - i * 4) * 8) =
            internal[index].bytes[j];
      }
      outputToLastStage.write(data_tmp);
      std::cout << name << " write Through data wired " << std::hex
                << std::setw(2) << std::setfill('0') << data_tmp.to_int()
                << std::endl;
      std::cout << name << " write Through data wired in binary "
                << data_tmp.to_string() << std::endl;
      addressToLastStage.write(addressToMem);
      wait();
      requestToLastStage.write(false);
      addressToMem.range(31, 0) = addressToMem.to_int() + 4;
    }
  }
  void printCacheLine(int index) {
    std::cout << name << " with cache line: " << index
              << ": tag: " << internal[index].tag << "|";
    for (int i = 0; i < cacheLineSize; i++) {
      std::cout << std::hex << std::setw(2) << std::setfill('0')
                << (int)internal[index].bytes[i] << "|";
    }
    std::cout << std::endl;
  }
  ~CACHEL2() {
    for (int i = 0; i < cacheLines; i++) {
      free(internal[i].bytes);
    }
    delete[] internal;
  }
};

#endif