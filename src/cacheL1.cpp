#ifndef CACEHL1_CPP
#define CACEHL1_CPP

#include <systemc>
#include <systemc.h>
#include <cmath>
#include <iostream>
#include <iomanip>
#include "types.h"
#include "cacheL2.cpp"

SC_MODULE(CACHEL1){
    int latency;
    int cacheLines; // this has also to be 2er Potenz
    
    // here i took maximal cacheLineSize as 128 bit, which is also the largest range in the industry，so the max of this Size is 16 Byte
    int cacheLineSize; 

    int miss;
    int hits;

    //tag|index|cacheOffset
    int offsetLength;
    int indexOffset; // the index position for a 32 bit address     
    int indexLength;
    int tagOffset;

    int tagbits; // the length of tagbits


    int waitingForLatency = 0; // the latency counter
    
    CacheLine* l2; // the internal storage of l2, for a faster communication with l2

    const char* name;
    
    sc_in<bool> clk;
    sc_in<bool> requestIncoming;
    sc_in<sc_bv<32>> inputData;
    sc_in<sc_bv<32>> address;
    sc_in<bool> rw;  

    // communication with last stage
    sc_in<bool> readyFromLastStage;
    sc_in<sc_bv<32>> dataFromLastStage; // Wichtig: for a better communication, this will be the index of the array in l2 storage
    sc_out<bool> requestToLastStage;
    sc_out<bool> rwToLastStage; // here can "last stage" be l2-cache
    sc_out<sc_bv<32>> addressToLastStage;
    sc_out<sc_bv<32>> outputToLastStage; // output forward to inside

    sc_out<bool> ready; // this component is ready to use
    sc_out<sc_bv<32>> outputData; // output to outside

    sc_out<bool> isWriteThrough; // the signal to l2, shows that if the current write Request a Write Through is
    

    CacheLine *internal; // the internal storage
    
    SC_CTOR(CACHEL1);
    CACHEL1(sc_module_name name, int latency, int cacheLines,int cacheLineSize) : sc_module(name) {
        this->latency = latency;
        this->cacheLines = cacheLines;
        this->cacheLineSize = cacheLineSize;
        waitingForLatency = 0;
        hits = 0;
        miss = 0;
        offsetLength = (int)(log(cacheLineSize)/log(2));
        indexLength = (int)(log(cacheLines)/log(2));
        tagbits = 32 - offsetLength-indexLength;

        tagOffset = 32-tagbits;
        indexOffset = 32-tagbits-indexLength;

        internal = new CacheLine[cacheLines];
        for(int i = 0; i < cacheLines; i++){
            internal[i].bytes = (uint8_t*)calloc (cacheLineSize,sizeof(uint8_t));
            internal[i].empty = 1;
        }

        this->name = (const char*)name;
        SC_THREAD(run);
        sensitive<<clk.pos()<<requestIncoming;
    }
    
    void run(){
        while(true){
            wait();
            if(requestIncoming.read()){ // upon request shall the component start to work    
                std::cout<<name<<" received request"<<std::endl;
                ready.write(false); 
                wait(latency*2,SC_NS);
                /*
                while(waitingForLatency<latency){ // latency counter
                    ready.write(false); 
                    wait();
                    //std::cout<<name<<" waiting for latency"<< std::endl;
                    waitingForLatency++;
                }     
                */
                if(rw.read()){ // write enable
                    std::cout<<name<<" received <write> with addr:"<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int()<< " and data:"<<inputData.read().to_int()<< std::endl;
                    write();
                }else{
                    std::cout<<name<<" received <read> with addr:"<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int()<< std::endl;
                    read();
                }
            }else{
                //std::cout<<name<<" waiting for request"<< std::endl;
                ready.write(true);
            }
            waitingForLatency = 0;
        }
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
        // check if the input address has different index (the lowest address, the highest address)
        sc_bv<32> addressBV_low = address.read();
        sc_bv<32> addressBV_high = addressBV_low.to_uint()+3;
        int offset_tmp = address.read().range(indexOffset-1,0).to_uint();

        if(offset_tmp<cacheLineSize-3){ // the accessed 4 Byte can be found in one Cache line
            std::cout<< "start dealing data that can be extracted from one line"<<std::endl;
            int t_tmp = address.read().range(31,tagOffset).to_uint();
            int i_tmp = address.read().range(tagOffset-1,indexOffset).to_uint();
            index = ifExist(t_tmp,i_tmp);

            if(index==-1){ // in the cache there are no such information
                std::cout<<name<<" miss by writing with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int()<< " detected, sending signal to next level"<< std::endl;
                miss++;
                hit = false;
                index = loadFromL2(address.read().to_uint());
            }
            if(hit){
                std::cout<<name<<" hit with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int() << std::endl;
                hits++;
            }

            // at this point, the required data is successfully loaded from last stage
            //calculate the offset and change the required data
            int offset = address.read().range(indexOffset-1,0).to_uint();
            //offset = offset & 0xfffffffc; // the address is always 4 Byte aligned //2.0 We dont need this alignment anymore
            for(int i = 0; i < 4; i++){
                internal[index%cacheLines].bytes[offset+i] = inputData.read().range(31-i*8,31-(i+1)*8+1).to_uint(); // big endian
            }
            
            writeThrough(index%cacheLines,address.read().to_uint());
            //wait(); // wait for lastStage to exicute and change the request Singnal to false
            requestToLastStage.write(false);
            ready.write(true);
            std::cout<<name<<" finished with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int() << std::endl;
        }else{ // the accessed 4 Byte has to be found in 2 Cache lines
            std::cout<< "start dealing data that can be extracted from 2 lines with lowest address: "<< std::hex << std::setw(8) << std::setfill('0')<<addressBV_low.to_uint()<<" and highest: "<< std::hex << std::setw(8) << std::setfill('0')<<addressBV_high.to_uint()<<std::endl;
            // first deal with the first cacheLine
            int t_tmp = addressBV_low.range(31,tagOffset).to_uint();
            int i_tmp = addressBV_low.range(tagOffset-1,indexOffset).to_uint();
            index = ifExist(t_tmp,i_tmp);
            if(index==-1){ // in the cache there are no such information
                std::cout<<name<<" miss by writing with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int()<< " detected, sending signal to next level"<< std::endl;
                hit = false; // if the first accessed cache line is missed, then shall this eventually be a miss
                index = loadFromL2(address.read().to_uint());
            }
            if(hit){
                std::cout<<name<<" hit with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int() << std::endl;
            }

            // at this point, the required data is successfully loaded from last stage
            //calculate the offset and change the required data
            int offset = address.read().range(indexOffset-1,0).to_uint();
            //offset = offset & 0xfffffffc; // the address is always 4 Byte aligned //2.0 We dont need this alignment anymore
            int i; // the number to record the written Bytes in the first line, so that we can continue to write the rest in the second
            for(i = 0; i+offset < cacheLineSize; i++){
                internal[index%cacheLines].bytes[offset+i] = inputData.read().range(31-i*8,31-(i+1)*8+1).to_uint();
            }
            printCacheLine(index%cacheLines);
            writeThrough(index%cacheLines,address.read().to_uint());
            //wait(); // wait for lastStage to exicute and change the request Singnal to false
            requestToLastStage.write(false);
            std::cout<<name<<" finished with loading the first line from l2 with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int() << std::endl;

            // second deal with the second cacheLine
            t_tmp = addressBV_high.range(31,tagOffset).to_uint();
            i_tmp = addressBV_high.range(tagOffset-1,indexOffset).to_uint();
            index = ifExist(t_tmp,i_tmp);
            if(index==-1){ // in the cache there are no such information
                std::cout<<name<<" miss by writing with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int()<< " detected, sending signal to next level"<< std::endl;
                hit = false; 
                index = loadFromL2(addressBV_high.to_uint());
            }
            if(hit){
                hits++;
                std::cout<<name<<" hit with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int() << std::endl;
            }
            else{
                miss++;
            }

            // at this point, the required data is successfully loaded from last stage
            //calculate the offset and change the required data
            //int offset = address.read().range(indexOffset-1,0).to_uint(); we dont need a offset here because the byte shall always be written from the beginning in the second line
            //offset = offset & 0xfffffffc; // the address is always 4 Byte aligned //2.0 We dont need this alignment anymore
            for(int j = i; j < 4; j++){
                internal[index%cacheLines].bytes[j-i] = inputData.read().range(31-j*8,31-(j+1)*8+1).to_uint();
            }
            printCacheLine(index%cacheLines);
            writeThrough(index%cacheLines,addressBV_high.to_uint());
            //wait(); // wait for lastStage to exicute and change the request Singnal to false
            requestToLastStage.write(false);
            ready.write(true);
            std::cout<<name<<" finished with loading the second line from l2 with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int() << std::endl;
        }
    }
    void read(){
        bool hit = true;
        int index = -1;
        // check if the input address has different index (the lowest address, the highest address)
        sc_bv<32> addressBV_low = address.read();
        sc_bv<32> addressBV_high = addressBV_low.to_uint()+3;
        int offset_tmp = address.read().range(indexOffset-1,0).to_uint();

        if(offset_tmp < cacheLineSize-3){ // the read data can be accessed in one cache line
            int t_tmp = address.read().range(31,tagOffset).to_uint();
            int i_tmp = address.read().range(tagOffset-1,indexOffset).to_uint();
            index = ifExist(t_tmp,i_tmp);
            if(index==-1){ // in the cache there are no such information
                std::cout<<name<<" miss by reading with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int()<< " detected, sending signal to next level"<< std::endl;
                miss++;
                hit = false;
                index = loadFromL2(address.read().to_int());
            }
            if(hit){
                std::cout<<name<<" hit with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int() << std::endl;
                hits++;
            }

            int offset = address.read().range(indexOffset-1,0).to_uint();
            //offset = offset & 0xfffffffc; alignment is no longer needed
            sc_bv<32> output_tmp;
            for(int i = 0; i< 4; i++){
                output_tmp.range(31-i*8,31-(i+1)*8+1) = internal[index%cacheLines].bytes[offset+i];
            }
            outputData.write(output_tmp); // here we are able to check if the read information is as expected
            ready.write(true);
            std::cout<<name<<" finished with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int() <<" with read data:"<<output_tmp.to_int()<< std::endl;
        }else{ // the accessed data has to be found in 2 cache line
            int t_tmp = addressBV_low.range(31,tagOffset).to_uint();
            int i_tmp = addressBV_low.range(tagOffset-1,indexOffset).to_uint();
            index = ifExist(t_tmp,i_tmp);
            if(index==-1){ // in the cache there are no such information
                std::cout<<name<<" miss by reading with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int()<< " detected, sending signal to next level"<< std::endl;
                hit = false; // if the first accessed cache line is missed, then shall this eventually be a miss
                index = loadFromL2(address.read().to_uint());
            }
            if(hit){
                std::cout<<name<<" hit with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int() << std::endl;
            }

            // at this point, the required data is successfully loaded from last stage
            //calculate the offset and change the required data

            // read data from the first cache line
            sc_bv<32> output_tmp;
            int offset = address.read().range(indexOffset-1,0).to_uint();
            int i = 0; // i for recording the number of the read bytes
            printCacheLine(index%cacheLines);
            for(i = 0; offset + i< cacheLineSize; i++){
                output_tmp.range(31-i*8,31-(i+1)*8+1) = internal[index%cacheLines].bytes[offset+i];
                std::cout<<name<<" current loaded: "<< std::hex << std::setw(8) << std::setfill('0')<<output_tmp.to_int()<< std::endl;
            }

            t_tmp = addressBV_high.range(31,tagOffset).to_uint();
            i_tmp = addressBV_high.range(tagOffset-1,indexOffset).to_uint();
            index = ifExist(t_tmp,i_tmp);
            if(index==-1){ // in the cache there are no such information
                std::cout<<name<<" miss by reading with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int()<< " detected, sending signal to next level"<< std::endl;
                hit = false; // if the first accessed cache line is missed, then shall this eventually be a miss
                index = loadFromL2(addressBV_high.to_uint());
            }
            if(hit){
                hits++;
                std::cout<<name<<" hit with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int() << std::endl;
            }else{
                miss++;
            }

            // at this point, the required data is successfully loaded from last stage
            //calculate the offset and change the required data
            // read data from the second line
            printCacheLine(index%cacheLines);
            for(int j = i; j< 4; j++){
                output_tmp.range(31-j*8,31-(j+1)*8+1) = internal[index%cacheLines].bytes[j - i]; // read from the beginning of the second line
                std::cout<<name<<" current loaded: "<< std::hex << std::setw(8) << std::setfill('0')<<output_tmp.to_int()<< std::endl;
            }

            outputData.write(output_tmp); // here we are able to check if the read information is as expected
            ready.write(true);
            std::cout<<name<<" finished with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int() <<" with read data:"<<output_tmp.to_int()<< std::endl;
        }
    }

    void writeThrough(int index,int addressToWrite){
        sc_bv<32> wiredAddr = addressToWrite;
        while(!readyFromLastStage.read()){ // wait until l2 is ready
            wait();
        }
        requestToLastStage.write(true);
        rwToLastStage.write(true);
        //by write Throught, we provide a index for l2 to gain immediate access to the whole cache line of l1
        outputToLastStage.write(index);
        addressToLastStage.write(wiredAddr);
        isWriteThrough.write(true);
        wait();
        requestToLastStage.write(false);
        isWriteThrough.write(false);
        wait();
    }
    int loadFromL2(int addressToLoad){
        while(!readyFromLastStage.read()){ // keep on waitin until last Stage is ready
            wait();
        }
        std::cout<<name<<" with last stage ready, start sending signal to last stage"<< std::endl;
        requestToLastStage.write(true);
        rwToLastStage.write(false);
        sc_bv<32> addWire = addressToLoad;
        addressToLastStage.write(addWire);
        std::cout<<name<<" sended addr: "<< std::hex << std::setw(8) << std::setfill('0')<< address.read().to_int()<< std::endl;
        wait();
        //std::cout<<name<<" readiness from L2: "<<readyFromLastStage.read()<< std::endl;
        requestToLastStage.write(false);
        wait(); // wait for the ready signal to change
        while(!readyFromLastStage.read()){ // keep on waitin until last Stage is ready
            wait();
        }
        std::cout<<name<<" with last stage ready for data preperation"<< std::endl;
        requestToLastStage.write(false);
        std::cout<<name<<" received data index from last stage: "<< std::hex << std::setw(8) << std::setfill('0')<< dataFromLastStage.read().to_int()<< std::endl;

        int index;
        if(indexLength == 0){
            index = 0;
        }
        else{
            index = addWire.range(tagOffset-1,indexOffset).to_uint();
        }
        int indexFromL2 = dataFromLastStage.read().to_int();
        // finish fetching from L2, start changing the internal data
        internal[index%cacheLines].tag = addWire.range(31,tagOffset).to_uint();
        for(int i = 0; i< cacheLineSize;i++){
            internal[index%cacheLines].bytes[i] = l2[indexFromL2].bytes[i];
        }
        internal[index%cacheLines].empty = 0;
        return index;
    }
    void printCacheLine(int index){
        std::cout<<name<<" with cache line: "<<index<<": tag: "<<internal[index].tag<<"|";
        for(int i = 0; i< cacheLineSize;i++){
            std::cout<< std::hex << std::setw(2) << std::setfill('0')<< (int)internal[index].bytes[i]<<"|";
        }
        std::cout<< std::endl;
    }
    ~CACHEL1(){
        for(int i = 0; i< cacheLines; i++){
            free(internal[i].bytes);
        }
        delete[] internal;
    } 
};

#endif