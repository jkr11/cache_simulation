#ifndef CACEHL2_CPP
#define CACEHL2_CPP

#include <systemc>
#include <systemc.h>
#include <cmath>
#include <iostream>
#include <iomanip>
#include "types.h"
#include "cacheL1.cpp"

//#define L2_DETAIL
#define HIT_LOG
//#define TIME_LOG
SC_MODULE(CACHEL2){
    int latency;
    int cacheLines; // this has also to be 2er Potenz
    int L1cacheLines;
    // here i took maximal cacheLineSize as 128 bit, which is also the largest range in the industry，so the max of this Size is 16 Byte
    int cacheLineSize; 

    int miss;
    int hits;

    CacheLine* l1; // the internal storage of l1

    // tag|index|Offset
    int offsetLength;
    int indexOffset; // the index position for a 32 bit address     
    int indexLength;
    int tagOffset;
    int tagbits; // the length of tagbits

    int waitingForLatency = 0;
    
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
    sc_in<sc_bv<32>> dataFromLastStage; // this will give the current index of the internal storage of l1
    sc_out<bool> requestToLastStage;
    sc_out<bool> rwToLastStage; 
    sc_out<sc_bv<32>> addressToLastStage;
    sc_out<sc_bv<32>> outputToLastStage; // output forward to inside

    sc_out<bool> ready; // this component is ready to use
    sc_out<sc_bv<32>> outputData; // output to outside
    

    // the internal storage
    CacheLine *internal;
    
    SC_CTOR(CACHEL2);
    CACHEL2(sc_module_name name, int latency, int cacheLines,int cacheLineSize, int L1CacheLines) : sc_module(name) {
        this->latency = latency;
        this->cacheLines = cacheLines;
        this->cacheLineSize = cacheLineSize;
        this->L1cacheLines = L1CacheLines;
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
            internal[i].bytes = (uint8_t*)calloc (cacheLineSize,sizeof(uint8_t));;
            internal[i].empty = 1;
        }

        this->name = (const char*)name;
        SC_THREAD(run);
        sensitive<<clk.pos()<<requestIncoming<<readyFromLastStage;
    }
    // as required the cache is full associative and shall be replaced with LRU
    void run(){
        while(true){
            wait();
        
        if(requestIncoming.read()){ // upon request shall the component start to work   
            #ifdef TIME_LOG
            std::cout<<name<<" received request: "<< requestIncoming.read()<<"at time: "<<sc_time_stamp()<<std::endl;
            #endif
            isWriteT = isWriteThrough.read(); 
            ready.write(false); 
            
                /*
                while(waitingForLatency<latency){ // latency counter
                    ready.write(false); 
                    wait();
                    //std::cout<<name<<" waiting for latency"<< std::endl;
                    waitingForLatency++;
                }     
                */  
            #ifdef TIME_LOG
            std::cout<<name<<" start working at: "<<sc_time_stamp()<<std::endl;
            #endif
            if(rw.read()){ // write enable
                /*#ifdef L2_DETAIL
                int receivedData = 0;
                for(int i = 0; i< cacheLineSize;i++){
                    receivedData += l1[inputData.read().to_int()].bytes[i];
                }
                std::cout<<name<<" received <write> with addr:"<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int()<< " and data:"<< std::hex << std::setw(8) << std::setfill('0')<<receivedData<< std::endl;
                #endif
                */
                write();// by writeThrough we only expect a latency of Memory, therefore the latency of write() is omitted
            }else{
                wait(latency*2,SC_NS); 
                #ifdef L2_DETAIL
                std::cout<<name<<" received <read> with addr:"<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int()<< std::endl;
                #endif
                read();
            }
            #ifdef TIME_LOG
            std::cout<<name<<" ready at time: "<<sc_time_stamp()<<std::endl;
            #endif
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

    void write(){ // this only happens at write-Through
        bool hit = true;
        int index = -1;
        int t_tmp = address.read().range(31,tagOffset).to_uint();
        int i_tmp = address.read().range(tagOffset-1,indexOffset).to_uint();
        index = ifExist(t_tmp,i_tmp); 
            if(index==-1){ // in the cache there are no such information // in Theory, this shall never happen, but for safty we kept these here
                #ifdef HIT_LOG
                std::cout<<name<<" miss by writing with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int()<< " detected, sending signal to next level"<< std::endl;
                #endif
                miss++;
                hit = false;
                index = loadFromMem();
            }
            if(hit&&isWriteT){
                #ifdef HIT_LOG
                std::cout<<name<<" hit with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int() << std::endl;
                #endif
                hits++;
            }
            // at this point, the required data is successfully loaded from last stage
            //calculate the offset and change the required data
            int offset_tmp = address.read().range(indexOffset-1,0).to_uint();
            
            if(offset_tmp < cacheLineSize-3){ // data lies within one line
                #ifdef L2_DETAIL
                std::cout<<name<<" with l1 index: "<<inputData.read().to_int() << std::endl;
                #endif
                for(int i = 0; i< cacheLineSize;i++){ // direct communication with l1

                    internal[index%cacheLines].bytes[i] = l1[inputData.read().to_int()].bytes[i];
                }
            }else{
                #ifdef L2_DETAIL
                std::cout<<name<<" with l1 index: "<<inputData.read().to_int() << std::endl;
                #endif
                for(int i = 0; i< cacheLineSize;i++){ // direct communication with l1
                    internal[index%cacheLines].bytes[i] = l1[inputData.read().to_int()].bytes[i];
                }
                for(int i = 0; i< cacheLineSize;i++){ // direct communication with l1
                    internal[(index+1)%cacheLines].bytes[i] = l1[(inputData.read().to_int()+1)%L1cacheLines].bytes[i];
                }
            }
            writeThrough(index%cacheLines);
            if(offset_tmp < cacheLineSize-3){
                wait(latency*2,SC_NS); 
            }else{
                wait(latency*2,SC_NS); 
                wait(latency*2,SC_NS); 
            }
            //wait(); //wait for lastStage to exicute and change the request Singnal to false
            requestToLastStage.write(false);
        	ready.write(true);
            #ifdef L2_DETAIL
            std::cout<<name<<" finished with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int() << std::endl;
            #endif
    }
    void read(){
        bool hit = true;
        int index = -1;
        int t_tmp = address.read().range(31,tagOffset).to_uint();
        int i_tmp = address.read().range(tagOffset-1,indexOffset).to_uint();
        index = ifExist(t_tmp,i_tmp);
        if(index==-1){ // in the cache there are no such information
            #ifdef HIT_LOG
            std::cout<<name<<" miss by reading with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int()<< " detected, sending signal to next level"<< std::endl;
            #endif
            miss++;
            hit = false;
            index = loadFromMem();
        }
        if(hit){
            #ifdef HIT_LOG
            std::cout<<name<<" hit with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int() << std::endl;
            #endif
            hits++;
        }
        outputData.write(index%cacheLines); // send the index of internal for l1 to gain immediate access to whole cache line
        ready.write(true);
        #ifdef L2_DETAIL
        std::cout<<name<<" finished with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int() << std::endl;
        #endif
    }

    void writeThrough(int index){
        int offset_tmp = address.read().range(indexOffset-1,0).to_uint();
        sc_bv<32> addressToMem = address.read();
        //addressToMem.range(offsetLength-1,0) = 0;//address alignment
        printCacheLine(index%cacheLines);
        printCacheLine((index+1)%cacheLines);
        sc_bv<32> data_tmp;
        if(readyFromLastStage.read()){
            wait(); // check if Memory havent got the chance to change
        }
        while(!readyFromLastStage.read()){
            wait();
        }
        #ifdef TIME_LOG
        std::cout<<name<<" write Through data at time:"<<sc_time_stamp()<<std::endl;
        #endif
        requestToLastStage.write(true);
        rwToLastStage.write(true);
        if(offset_tmp < cacheLineSize-3){ // load data from 1 line
            for(int j = 0; j < 4; j++){
                //std::cout<<name<<" write Through data loaded "<< std::hex << std::setw(2) << std::setfill('0')<<(uint32_t)internal[index].bytes[j] << std::endl;
                data_tmp.range((j+1)*8-1,j*8) = internal[index].bytes[offset_tmp+j];
            }
        }else{
            int j;
            for(j = 0; offset_tmp+j < cacheLineSize; j++){
                //std::cout<<name<<" write Through data loaded "<< std::hex << std::setw(2) << std::setfill('0')<<(uint32_t)internal[index].bytes[j] << std::endl;
                data_tmp.range((j+1)*8-1,j*8) = internal[index].bytes[offset_tmp+j];
            }
            for(int i = j; i<4;i++){
                data_tmp.range((i+1)*8-1,i*8) = internal[(index+1)%cacheLines].bytes[i-j];
            }
        }
        
        outputToLastStage.write(data_tmp);
        #ifdef L2_DETAIL
        std::cout<<name<<" write Through data wired "<< std::hex << std::setw(2) << std::setfill('0')<<data_tmp.to_int() << std::endl;
        std::cout<<name<<" write Through data wired in binary "<<data_tmp.to_string() << std::endl;
        #endif
        addressToLastStage.write(addressToMem);
        wait();
        //std::cout<<name<<" current 220 at time:"<<sc_time_stamp()<<std::endl;
        requestToLastStage.write(false);
        //while(!readyFromLastStage.read()){ // wait until last stage is ready with write
        //    wait();
        //}
        //wait(); // wait for the ready from mem to change
        //std::cout<<name<<" current 223 at time:"<<sc_time_stamp()<<std::endl;
        #ifdef TIME_LOG
        std::cout<<name<<" finished write Through data at time:"<<sc_time_stamp()<<std::endl;
        #endif
    }
    void printCacheLine(int index){
        #ifdef L2_DETAIL
        std::cout<<name<<" with cache line: "<<index<<": tag: "<<internal[index].tag<<"|";
        for(int i = 0; i< cacheLineSize;i++){
            std::cout<< std::hex << std::setw(2) << std::setfill('0')<< (int)internal[index].bytes[i]<<"|";
        }
        std::cout<< std::endl;
        #endif
        (void) index;
    }
    int loadFromMem(){
        sc_bv<32> addressToMem; 
        int index = address.read().range(tagOffset-1,indexOffset).to_uint();;
        addressToMem.range(31,0) = address.read().to_int();
        addressToMem.range(offsetLength-1,0) = 0; // align with the starting address with each line
        for(int i = 0; i< (int)(cacheLineSize/4);i++){
            while(!readyFromLastStage.read()){ // keep on waitin until last Stage is ready
                wait();
            }
            #ifdef TIME_LOG
            std::cout<<name<<" with last stage ready, start sending number "<<i<<". signal to last stage at time: "<<sc_time_stamp()<< std::endl;
            #endif
            requestToLastStage.write(true);
            rwToLastStage.write(false);
            #ifdef L2_DETAIL
            std::cout<<name<<" aligned addr: "<< std::hex << std::setw(8) << std::setfill('0')<< addressToMem.to_int()<< std::endl;
            #endif
            addressToLastStage.write(addressToMem); // address alignment with 4
            #ifdef L2_DETAIL
            std::cout<<name<<" sended addr: "<< std::hex << std::setw(8) << std::setfill('0')<< address.read().to_int()<< std::endl;
            #endif
            wait();
            requestToLastStage.write(false);
            wait(); // wait for ready from mem to change
            while(!readyFromLastStage.read()){ // keep on waitin until last Stage is ready
                wait();
            }
            #ifdef TIME_LOG
            std::cout<<name<<" with last stage ready for number "<<i<< ". data preperation at time:"<<sc_time_stamp()<< std::endl;
            #endif
            requestToLastStage.write(false);
            #ifdef L2_DETAIL
            std::cout<<name<<" received data from last stage: "<< std::hex << std::setw(8) << std::setfill('0')<< dataFromLastStage.read().to_int()<< std::endl;
            #endif
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
        }
        delete[] internal;
    } 
};

#endif