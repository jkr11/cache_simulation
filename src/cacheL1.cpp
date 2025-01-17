#ifndef CACHEL1_CPP
#define CACHEL1_CPP

#include <systemc>
#include <systemc.h>
#include <cmath>
#include <iostream>
#include <iomanip>
#include "types.h"
#include "cacheL2.cpp"


SC_MODULE(CACHEL1 final) {
  int latency;
  int cacheLines;

  int cacheLineSize;

  int miss;
  int hits;

  //tag|index|cacheOffset
  int offsetLength;
  int indexOffset; // the index position for a 32 bit address
  int indexLength;
  int tagOffset;

  int tagbits; // the length of tagbits


  CacheLine* l2; // the internal storage of l2, for a faster communication with l2

  const char* name;

  sc_in<bool> clk;
  sc_in<bool> requestIncoming;
  sc_in<sc_bv<32>> inputData;
  sc_in<sc_bv<32>> address;
  sc_in<bool> rw;

  // communication with last stage
  sc_in<bool> readyFromLastStage;
  sc_in<sc_bv<32>> dataFromLastStage;
  // Wichtig: for a better communication, this will be the index of the array in l2 storage
  sc_out<bool> requestToLastStage;
  sc_out<bool> rwToLastStage; // here can "last stage" be l2-cache
  sc_out<sc_bv<32>> addressToLastStage;
  sc_out<sc_bv<32>> outputToLastStage; // output forward to inside

  sc_out<bool> ready; // this component is ready to use
  sc_out<sc_bv<32>> outputData; // output to outside

  sc_out<bool> isWriteThrough; // the signal to l2, shows that if the current write Request a Write Through is


  CacheLine* internal; // the internal storage

  SC_CTOR(CACHEL1);

  CACHEL1(const sc_module_name& name, const int latency, const int cacheLines,
          const int cacheLineSize) : sc_module(name) {
    this->latency = latency;
    this->cacheLines = cacheLines;
    this->cacheLineSize = cacheLineSize;
    hits = 0;
    miss = 0;
    offsetLength = static_cast<int>(log(cacheLineSize) / log(2));
    indexLength = static_cast<int>(log(cacheLines) / log(2));
    tagbits = 32 - offsetLength - indexLength;

    tagOffset = 32 - tagbits;
    indexOffset = 32 - tagbits - indexLength;

    internal = new CacheLine[cacheLines];
    for (int i = 0; i < cacheLines; i++) {
      internal[i].bytes = static_cast<uint8_t*>(calloc(cacheLineSize, sizeof(uint8_t)));
      internal[i].empty = 1;
    }

    this->name = static_cast<const char*>(name);
    SC_THREAD(run);
    sensitive << clk.pos() << requestIncoming << readyFromLastStage;
  }

  /**
   * @brief this Method is the entry point of L1Cache, it listen to the clock and requestIncomming then calls the requested method(read or write)
  */
  [[noreturn]] void run() {
    while (true) {
      wait();
      if (requestIncoming.read()) { // upon request shall the component start to work
        ready.write(false);
        if (rw.read()) { // write enable
          write();
        }
        else {
          read();
        }
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
  int ifExist(const int tag, const int index) const { //
    if (internal[index % cacheLines].empty == 1) {
      return -1;
    }
    const int tag_tmp = internal[index % cacheLines].tag;
    if (tag_tmp == tag)
      return index;
    return -1;
  }

  /**
   * @brief this method is the main logic of the write function.
   * It firstly check if the accessed data lies within 1 line or 2 lines, then load the required data to internal Storage from L2
   * After loaded or hits, it will perform a data Write and send Signal to L2, that all the modules can be writen.
   * The latency of L1 by writing will firstly be counted when its assured that all data are loaded in L1 internal Storage
  */
  void write() {
    bool hit = true;
    int index = -1;
    // check if the input address has different index (the lowest address, the highest address)
    sc_bv<32> addressBV_low = address.read();
    sc_bv<32> addressBV_high = addressBV_low.to_uint() + 3;
    const int offset_tmp = address.read().range(indexOffset - 1, 0).to_uint();

    if (offset_tmp < cacheLineSize - 3) { // the accessed 4 Byte can be found in one Cache line

      const int t_tmp = address.read().range(31, tagOffset).to_uint();
      const int i_tmp = address.read().range(tagOffset - 1, indexOffset).to_uint();
      index = ifExist(t_tmp, i_tmp);

      if (index == -1) { // in the cache there are no such information
        miss++;
        hit = false;
        index = loadFromL2(address.read().to_uint());
      }
      if (hit) {
        hits++;
      }

      // at this point, the required data is successfully loaded from last stage
      //calculate the offset and change the required data
      int offset = address.read().range(indexOffset - 1, 0).to_uint();
      for (int i = 0; i < 4; i++) {
        internal[index % cacheLines].bytes[offset + i] = inputData.read().range(31 - i * 8, 31 - (i + 1) * 8 + 1).
                                                                   to_uint(); // big endian
      }

      writeThrough(index % cacheLines, address.read().to_uint(), hit);
      requestToLastStage.write(false);
      wait(latency * 2, SC_NS);
      ready.write(true);
    }
    else { // the accessed 4 Byte has to be found in 2 Cache lines
      // first deal with the first cacheLine
      int t_tmp = addressBV_low.range(31, tagOffset).to_uint();
      int i_tmp = addressBV_low.range(tagOffset - 1, indexOffset).to_uint();
      index = ifExist(t_tmp, i_tmp);
      if (index == -1) { // in the cache there are no such information
        hit = false; // if the first accessed cache line is missed, then shall this eventually be a miss
        index = loadFromL2(address.read().to_uint());
      }

      // at this point, the required data is successfully loaded from last stage
      //calculate the offset and change the required data
      const int offset = address.read().range(indexOffset - 1, 0).to_uint();

      int i;
      // the number to record the written Bytes in the first line, so that we can continue to write the rest in the second
      for (i = 0; i + offset < cacheLineSize; i++) {
        internal[index % cacheLines].bytes[offset + i] = inputData.read().range(31 - i * 8, 31 - (i + 1) * 8 + 1).
                                                                   to_uint();
      }

      int oldIndex = index;
      requestToLastStage.write(false);

      // second deal with the second cacheLine
      t_tmp = addressBV_high.range(31, tagOffset).to_uint();
      i_tmp = addressBV_high.range(tagOffset - 1, indexOffset).to_uint();
      index = ifExist(t_tmp, i_tmp);
      if (index == -1) { // in the cache there are no such information
        hit = false;
        index = loadFromL2(addressBV_high.to_uint());
      }

      if (hit) {
        hits++;
      }
      else {
        miss++;
      }

      // at this point, the required data is successfully loaded from last stage
      //calculate the offset and change the required data
      for (int j = i; j < 4; j++) {
        internal[index % cacheLines].bytes[j - i] = inputData.read().range(31 - j * 8, 31 - (j + 1) * 8 + 1).to_uint();
      }

      writeThrough(oldIndex, address.read().to_uint(), hit);
      requestToLastStage.write(false);
      wait(latency * 2, SC_NS);
      wait(latency * 2, SC_NS);
      ready.write(true);
    }
  }

  /**
   * @brief this method is the main logic of the read function.
   * It firstly check if the accessed data lies within 1 line or 2 lines, then load the required data to internal Storage from L2
   * After loaded or hits, it will perform a data Read from one or two lines and send Ready to outside, that the data required is ready.
   * The latency of L1 by reading will be counted whenever there is a request
  */
  void read() {
    bool hit = true;
    int index = -1;
    // check if the input address has different index (the lowest address, the highest address)
    sc_bv<32> addressBV_low = address.read();
    sc_bv<32> addressBV_high = addressBV_low.to_uint() + 3;
    const int offset_tmp = address.read().range(indexOffset - 1, 0).to_uint();

    if (offset_tmp < cacheLineSize - 3) { // the read data can be accessed in one cache line
      wait(latency * 2, SC_NS);
      const int t_tmp = address.read().range(31, tagOffset).to_uint();
      const int i_tmp = address.read().range(tagOffset - 1, indexOffset).to_uint();
      index = ifExist(t_tmp, i_tmp);
      if (index == -1) { // in the cache there are no such information
        miss++;
        hit = false;
        index = loadFromL2(address.read().to_int());
      }
      if (hit) {
        hits++;
      }

      const int offset = address.read().range(indexOffset - 1, 0).to_uint();
      sc_bv<32> output_tmp;
      for (int i = 0; i < 4; i++) {
        output_tmp.range(31 - i * 8, 31 - (i + 1) * 8 + 1) = internal[index % cacheLines].bytes[offset + i];
      }
      outputData.write(output_tmp); // here we are able to check if the read information is as expected
      ready.write(true);
    }
    else { // the accessed data has to be found in 2 cache line
      int t_tmp = addressBV_low.range(31, tagOffset).to_uint();
      int i_tmp = addressBV_low.range(tagOffset - 1, indexOffset).to_uint();
      index = ifExist(t_tmp, i_tmp);
      wait(latency * 2, SC_NS);
      if (index == -1) { // in the cache there are no such information
        hit = false; // if the first accessed cache line is missed, then shall this eventually be a miss
        index = loadFromL2(address.read().to_uint());
      }
      // at this point, the required data is successfully loaded from last stage
      //calculate the offset and change the required data

      // read data from the first cache line
      sc_bv<32> output_tmp;
      const int offset = address.read().range(indexOffset - 1, 0).to_uint();
      int i = 0; // i for recording the number of the read bytes

      for (i = 0; offset + i < cacheLineSize; i++) {
        output_tmp.range(31 - i * 8, 31 - (i + 1) * 8 + 1) = internal[index % cacheLines].bytes[offset + i];
      }

      t_tmp = addressBV_high.range(31, tagOffset).to_uint();
      i_tmp = addressBV_high.range(tagOffset - 1, indexOffset).to_uint();
      index = ifExist(t_tmp, i_tmp);
      wait(latency * 2, SC_NS);
      if (index == -1) { // in the cache there are no such information
        hit = false; // if the first accessed cache line is missed, then shall this eventually be a miss
        index = loadFromL2(addressBV_high.to_uint());
      }

      if (hit) {
        hits++;
      }
      else {
        miss++;
      }

      // at this point, the required data is successfully loaded from last stage
      //calculate the offset and change the required data
      // read data from the second line

      for (int j = i; j < 4; j++) {
        output_tmp.range(31 - j * 8, 31 - (j + 1) * 8 + 1) = internal[index % cacheLines].bytes[j - i];
        // read from the beginning of the second line
      }

      outputData.write(output_tmp); // here we are able to check if the read information is as expected
      ready.write(true);
    }
  }

  /**
   * @brief this method communicate with L2 to inform that the data is ready to be written in all modules.
   */
  void writeThrough(const int index, const int addressToWrite, const bool hit) {
    // hit here shows if we need a writeThrough
    const sc_bv<32> wiredAddr = addressToWrite;
    while (!readyFromLastStage.read()) { // wait until l2 is ready
      wait();
    }
    if (hit) {
      isWriteThrough.write(true);
    }
    requestToLastStage.write(true);
    rwToLastStage.write(true);
    //by write Throught, we provide a index for l2 to gain immediate access to the whole cache line of l1
    outputToLastStage.write(index);
    addressToLastStage.write(wiredAddr);
    wait();
    requestToLastStage.write(false);
    isWriteThrough.write(false);
  }

  /**
   * @brief method load the required data from L2. It firstly send a requestToL2 signal with all necessary information,
   * after L2 is ready, the data shall be loaded into L1 internal Storage
   */
  int loadFromL2(const int addressToLoad) {
    while (!readyFromLastStage.read()) { // keep on waitin until last Stage is ready
      wait();
    }

    requestToLastStage.write(true);
    rwToLastStage.write(false);
    sc_bv<32> addWire = addressToLoad;
    addressToLastStage.write(addWire);
    wait();
    requestToLastStage.write(false);
    wait(); // wait for the ready signal to change
    while (!readyFromLastStage.read()) { // keep on waitin until last Stage is ready
      wait();
    }
    // start reading data from L2
    int index;
    if (indexLength == 0) {
      index = 0;
    }
    else {
      index = addWire.range(tagOffset - 1, indexOffset).to_uint();
    }
    const int indexFromL2 = dataFromLastStage.read().to_int();
    // finish fetching from L2, start changing the internal data
    internal[index % cacheLines].tag = addWire.range(31, tagOffset).to_uint();
    for (int i = 0; i < cacheLineSize; i++) {
      internal[index % cacheLines].bytes[i] = l2[indexFromL2].bytes[i];
    }
    internal[index % cacheLines].empty = 0;
    return index;
  }

  ~CACHEL1() {
    for (int i = 0; i < cacheLines; i++) {
      free(internal[i].bytes);
    }
    delete[] internal;
  }
};

#endif
