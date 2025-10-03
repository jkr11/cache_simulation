#ifndef CACHEL2_CPP
#define CACHEL2_CPP

#include <systemc>
#include <cmath>

#include "../include/types.h"

//#define L2_DETAIL
//#define HIT_LOG
//#define TIME_LOG
using namespace sc_core;
using namespace sc_dt;
SC_MODULE(CACHEL2 final) {
  int latency;
  int cacheLines;
  int L1cacheLines;
  int cacheLineSize;

  int miss;
  int hits;

  CacheLine* l1; // the internal storage of l1

  int offsetLength;
  int indexOffset; // the index position for a 32 bit address
  int indexLength;
  int tagOffset;
  int tagbits; // the length of tagbits

  bool isWriteT;

  const char* name;
  sc_in<bool> isWriteThrough; // if this is a write through request, miss will count but not the hit later in writing
  sc_in<bool> clk;
  sc_in<bool> requestIncoming;
  sc_in<sc_bv<32>> inputData;
  sc_in<sc_bv<32>> address;
  sc_in<bool> rw;


  // communication with last stage
  sc_in<bool> readyFromLastStage;
  sc_in<sc_bv<32>> dataFromLastStage; // this will show the current index of the internal storage of l1
  sc_out<bool> requestToLastStage;
  sc_out<bool> rwToLastStage;
  sc_out<sc_bv<32>> addressToLastStage;
  sc_out<sc_bv<32>> outputToLastStage; // output forward to inside

  sc_out<bool> ready; // this component is ready to use
  sc_out<sc_bv<32>> outputData; // output to outside


  // the internal storage
  CacheLine* internal;

  SC_CTOR(CACHEL2);

  CACHEL2(const sc_module_name& name, const int latency, const int cacheLines, const int cacheLineSize,
          const int L1CacheLines) : sc_module(name) {
    this->latency = latency;
    this->cacheLines = cacheLines;
    this->cacheLineSize = cacheLineSize;
    this->L1cacheLines = L1CacheLines;
    hits = 0;
    miss = 0;
    offsetLength = static_cast<int>(log(cacheLineSize) / log(2));
    indexLength = static_cast<int>(log(cacheLines) / log(2));
    tagbits = 32 - offsetLength - indexLength;
    tagOffset = 32 - tagbits;

    indexOffset = 32 - tagbits - indexLength;

    internal = new CacheLine[cacheLines];
    for (int i = 0; i < cacheLines; i++) {
      internal[i].bytes = (uint8_t*)calloc(cacheLineSize, sizeof(uint8_t));;
      internal[i].empty = 1;
    }

    this->name = (const char*)name;
    SC_THREAD(run);
    sensitive << clk.pos() << requestIncoming << readyFromLastStage;
  }

  /**
   * @brief this Method is the entry point of L2Cache, it listen to the clock and requestIncomming then calls the requested method(read or write)
   * the latency of read will be counted here, because all the read request forwarded to L2 are performed in 1 line
  */
  void run() {
    while (true) {
      wait();

      if (requestIncoming.read()) { // upon request shall the component start to work

        isWriteT = isWriteThrough.read();
        ready.write(false);

        if (rw.read()) { // write enable
          write(); // by writeThrough we only expect a latency of Memory, therefore the latency of write() is omitted
        }
        else {
          wait(latency * 2, SC_NS);
          read();
        }
#ifdef TIME_LOG
                std::cout<<name<<" ready at time: "<<sc_time_stamp()<<std::endl;
#endif
      }
      else {
        ready.write(true);
      }
    }
  }

  /**
   * @brief this method checks if the given address exist in the cache or is empty, returns the corresponding array index, and -1 if not exist
   * @param tag : the tag to be compared
   * @param index : the index to be compared
   * @return if the accessed date is valid in Cache, it will return its index. if not, -1
  */
  int ifExist(int tag, int index) {
    if (internal[index % cacheLines].empty == 1) {
      return -1;
    }
    int tag_tmp = internal[index % cacheLines].tag;
    if (tag_tmp == tag)
      return index;
    return -1;
  }

  /**
   * @brief this method is the main logic of the write function.
   * In Theory, L2 will only get a write request from L1 when its a WriteThrough, which means, if L1 is ready to write, so is L2.
   * Therefore there is no need ever to load from Memory
   * It firstly check if the accessed data lies within 1 line or 2 lines, then Write the required data to internal Storage
   * After recevied the the request signal,it will send Signal to Memory, that all the modules can be writen. It will then perform a data Write
   * The latency of L1 by writing will be counted according to whether its a one-line-access or two-line-access
  */
  void write() { // this only happens at write-Through
    bool hit = true;
    int index = -1;
    int t_tmp = address.read().range(31, tagOffset).to_uint();
    int i_tmp = address.read().range(tagOffset - 1, indexOffset).to_uint();
    index = ifExist(t_tmp, i_tmp);
    if (index == -1) {
      // in the cache there are no such information----- in Theory, this shall never happen, but for safty we kept these here
      miss++;
      hit = false;
      index = loadFromMem();
    }
    if (hit && isWriteT) {
      hits++;
    }
    // at this point, the required data is successfully loaded from last stage
    //calculate the offset and change the required data
    int offset_tmp = address.read().range(indexOffset - 1, 0).to_uint();

    if (offset_tmp < cacheLineSize - 3) { // data lies within one line
      for (int i = 0; i < cacheLineSize; i++) { // direct communication with l1

        internal[index % cacheLines].bytes[i] = l1[inputData.read().to_int()].bytes[i];
      }
    }
    else {
      for (int i = 0; i < cacheLineSize; i++) { // direct communication with l1
        internal[index % cacheLines].bytes[i] = l1[inputData.read().to_int()].bytes[i];
      }
      for (int i = 0; i < cacheLineSize; i++) { // direct communication with l1
        internal[(index + 1) % cacheLines].bytes[i] = l1[(inputData.read().to_int() + 1) % L1cacheLines].bytes[i];
      }
    }
    writeThrough(index % cacheLines);
    if (offset_tmp < cacheLineSize - 3) {
      wait(latency * 2, SC_NS);
    }
    else {
      wait(latency * 2, SC_NS);
      wait(latency * 2, SC_NS);
    }
    requestToLastStage.write(false);
    ready.write(true);
  }

  /**
   * @brief this method is the main logic of the read function.
   * It checks if the requested date lies within its internal Storage, if not load the required data to internal Storage from Memory
   * After loaded or hits, it will send Ready and the corresponding data to L1.
  */
  void read() {
    bool hit = true;
    int index = -1;
    int t_tmp = address.read().range(31, tagOffset).to_uint();
    int i_tmp = address.read().range(tagOffset - 1, indexOffset).to_uint();
    index = ifExist(t_tmp, i_tmp);
    if (index == -1) { // in the cache there are no such information
      miss++;
      hit = false;
      index = loadFromMem();
    }
    if (hit) {
      hits++;
    }
    outputData.write(index % cacheLines);
    // send the index of internal for l1 to gain immediate access to whole cache line
    ready.write(true);
  }

  /**
   * @brief this method communicate with Memory to inform that the data is ready to be written in all modules.
   */
  void writeThrough(int index) {
    int offset_tmp = address.read().range(indexOffset - 1, 0).to_uint();
    sc_bv<32> addressToMem = address.read();
    sc_bv<32> data_tmp;

    if (readyFromLastStage.read()) {
      wait(); // check if Memory havent got the chance to change
    }
    while (!readyFromLastStage.read()) {
      wait();
    }


    if (offset_tmp < cacheLineSize - 3) { // get output Data ready
      for (int j = 0; j < 4; j++) {
        data_tmp.range((j + 1) * 8 - 1, j * 8) = internal[index].bytes[offset_tmp + j];
      }
    }
    else { // load data from 2 lines
      int j;
      for (j = 0; offset_tmp + j < cacheLineSize; j++) {
        data_tmp.range((j + 1) * 8 - 1, j * 8) = internal[index].bytes[offset_tmp + j];
      }
      for (int i = j; i < 4; i++) {
        data_tmp.range((i + 1) * 8 - 1, i * 8) = internal[(index + 1) % cacheLines].bytes[i - j];
      }
    }

    requestToLastStage.write(true);
    rwToLastStage.write(true);
    outputToLastStage.write(data_tmp);
    addressToLastStage.write(addressToMem);
    wait();
    requestToLastStage.write(false);
  }

  /**
   * @brief method load the required data from Memory.
   * The requests are done by a 4-Byte-step from the beginning address of the required line to the last 4 Byte of this line, for the Databus is 4 Byte long.
   */
  int loadFromMem() {
    sc_bv<32> addressToMem;
    int index = address.read().range(tagOffset - 1, indexOffset).to_uint();;
    addressToMem.range(31, 0) = address.read().to_int();
    addressToMem.range(offsetLength - 1, 0) = 0; // align with the starting address with each line
    for (int i = 0; i < (int)(cacheLineSize / 4); i++) { // the loop the fill all the 4 Byte Block in a CacheLine
      while (!readyFromLastStage.read()) { // keep on waitin until last Stage is ready
        wait();
      }

      requestToLastStage.write(true);
      rwToLastStage.write(false);
      addressToLastStage.write(addressToMem); // address alignment with 4
      wait();
      requestToLastStage.write(false);
      wait(); // wait for ready from mem to change
      while (!readyFromLastStage.read()) { // keep on waitin until last Stage is ready
        wait();
      }

      requestToLastStage.write(false);

      internal[index % cacheLines].tag = address.read().range(31, tagOffset).to_uint();
      for (int j = i * 4; j < (i + 1) * 4; j++) {
        internal[index % cacheLines].bytes[j] = dataFromLastStage.read().range((j - i * 4 + 1) * 8 - 1, (j - i * 4) * 8)
                                                                 .to_uint();
      }
      internal[index % cacheLines].empty = 0;
      // increment of the address by 4
      addressToMem.range(31, 0) = addressToMem.to_int() + 4;
    }
    internal[index % cacheLines].empty = 0;
    return index;
  }

  ~CACHEL2() {
    for (int i = 0; i < cacheLines; i++) {
      free(internal[i].bytes);
    }
    delete[] internal;
  }
};

#endif
