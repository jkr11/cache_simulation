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
            internal[i].bytes = (uint8_t*)malloc (cacheLineSize*sizeof(uint8_t));
            internal[i].empty = 1;
        }

        this->name = (const char*)name;
        SC_THREAD(run);
        sensitive<<clk.pos()<<requestIncoming;
    }
    
    void run(){
        while(true){
        wait();
        if(requestIncoming){ // upon request shall the component start to work    
            std::cout<<name<<" received request"<<std::endl;
            while(waitingForLatency<latency){ // latency counter
                ready.write(false); 
                wait();
                //std::cout<<name<<" waiting for latency"<< std::endl;
                waitingForLatency++;
            }     
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
        int t_tmp = address.read().range(31,tagOffset).to_uint();
        int i_tmp = address.read().range(tagOffset-1,indexOffset).to_uint();
        index = ifExist(t_tmp,i_tmp);
            if(index==-1){ // in the cache there are no such information
                std::cout<<name<<" miss by writing with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int()<< " detected, sending signal to next level"<< std::endl;
                miss++;
                hit = false;
                while(!readyFromLastStage.read()){ // keep on waitin until last Stage is ready
                    wait();
                }
                std::cout<<name<<" with last stage ready, start sending signal to last stage"<< std::endl;
                requestToLastStage.write(true);
                rwToLastStage.write(false);
                addressToLastStage.write(address.read());
                std::cout<<name<<" sended addr: "<< std::hex << std::setw(8) << std::setfill('0')<< address.read().to_int()<< std::endl;
                wait();
                requestToLastStage.write(false);
                while(!readyFromLastStage.read()){ // keep on waitin until last Stage is ready
                    wait();
                }
                std::cout<<name<<" with last stage ready for data preperation"<< std::endl;
                requestToLastStage.write(false);
                std::cout<<name<<" received data index from last stage: "<< std::hex << std::setw(8) << std::setfill('0')<< dataFromLastStage.read().to_int()<< std::endl;
                if(indexLength == 0){
                    index = 0;
                }
                else{
                    index = address.read().range(tagOffset-1,indexOffset).to_uint();
                }
                int indexFromL2 = dataFromLastStage.read().to_int();
                // finish fetching from L2, start changing the internal data
                internal[index%cacheLines].tag = address.read().range(31,tagOffset).to_uint();
                for(int i = 0; i< cacheLineSize;i++){
                    internal[index%cacheLines].bytes[i] = l2[indexFromL2].bytes[i];
                }
                internal[index%cacheLines].empty = 0;
            }
            if(hit){
                std::cout<<name<<" hit with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int() << std::endl;
                hits++;
            }
            // at this point, the required data is successfully loaded from last stage
            //calculate the offset and change the required data
            int offset = address.read().range(indexOffset-1,0).to_uint();
            offset = offset & 0xfffffffc; // the address is always 4 Byte aligned
            for(int i = 0; i < 4; i++){
                internal[index%cacheLines].bytes[offset+i] = inputData.read().range((i+1)*8-1,i*8).to_uint();
            }
            
            writeThrough(index%cacheLines);
            wait(); // wait for lastStage to exicute and change the request Singnal to false
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
                while(!readyFromLastStage.read()){ // keep on waitin until last Stage is ready
                    wait();
                }
                std::cout<<name<<" with last stage ready, start sending signal to last stage"<< std::endl;
                requestToLastStage.write(true);
                rwToLastStage.write(false);
                addressToLastStage.write(address.read());
                std::cout<<name<<" sended addr: "<< std::hex << std::setw(8) << std::setfill('0')<< addressToLastStage.read().to_int()<< std::endl;
                wait();
                requestToLastStage.write(false);
                while(!readyFromLastStage.read()){ // keep on waitin until last Stage is ready
                    wait();
                }
                std::cout<<name<<" with last stage ready for data preperation"<< std::endl;
                requestToLastStage.write(false);
                std::cout<<name<<" received data index from last stage: "<< std::hex << std::setw(8) << std::setfill('0')<< dataFromLastStage.read().to_int()<< std::endl;
                if(indexLength == 0){
                    index = 0;
                }
                else{
                    index = address.read().range(tagOffset-1,indexOffset).to_uint();
                }
                int indexFromL2 = dataFromLastStage.read().to_int();
                // finish fetching from L2, start changing the internal data
                internal[index%cacheLines].tag = address.read().range(31,tagOffset).to_uint();
                for(int i = 0; i< cacheLineSize;i++){
                    internal[index%cacheLines].bytes[i] = l2[indexFromL2].bytes[i];
                }
                internal[index%cacheLines].empty = 0;
            }
            if(hit){
                std::cout<<name<<" hit with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int() << std::endl;
                hits++;
            }

            int offset = address.read().range(indexOffset-1,0).to_uint();
            offset = offset & 0xfffffffc;
            sc_bv<32> output_tmp;
            for(int i = 0; i< 4; i++){
                output_tmp.range((i+1)*8-1,i*8) = internal[index%cacheLines].bytes[offset+i];
            }
            outputData.write(output_tmp); // here we are able to check if the read information is as expected
            ready.write(true);
            std::cout<<name<<" finished with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int() <<" with read data:"<<output_tmp.to_int()<< std::endl;
    }

    void writeThrough(int index){
        while(!readyFromLastStage.read()){ // wait until l2 is ready
            wait();
        }
        requestToLastStage.write(true);
        rwToLastStage.write(true);
        //by write Throught, we provide a index for l2 to gain immediate access to the whole cache line of l1
        outputToLastStage.write(index);
        addressToLastStage.write(address.read());
        isWriteThrough.write(true);
        wait();
        requestToLastStage.write(false);
        isWriteThrough.write(false);
    }
    ~CACHEL1(){
        for(int i = 0; i< cacheLines; i++){
            free(internal[i].bytes);
        }
        delete[] internal;
    } 
};

#endif