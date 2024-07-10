#include <systemc>
#include <systemc.h>
#include <cmath>
#include <iostream>
#include <iomanip>
// here we took assumption that the cacheLineSize mus be greater than 4, for our request comes in 4 bytes
// currently we took "unaligned access" out of consideration
SC_MODULE(CACHEL2){
    int latency;
    int cacheLines; // this has also to be 2er Potenz
    
    // here i took maximal cacheLineSize as 128 bit, which is also the largest range in the industry，so the max of this Size is 16 Byte
    int cacheLineSize; 

    int miss;
    int hits;
    int usageCount;

    int cacheOffset; // the "offset" for a 32 bit address            tag|index|cacheOffset
    int offsetLength;
    int indexOffset; // the index position for a 32 bit address     
    int indexLength;
    int tagOffset;
    int tagOffsetInCache;
    int tagbits; // the length of tagbits
    int dataOffset; // data offset for a cache line    tag|data

    int waitingForLatency = 0;
    
    bool isWriteT;

    const char* name;
    sc_in<bool> isWriteThrough;
    sc_in<bool> clk;
    sc_in<bool> requestIncoming;
    sc_in<sc_bv<32>> inputData;
    sc_in<sc_bv<32>> address;
    sc_in<bool> rw;  

    // communication with last stage
    sc_in<bool> readyFromLastStage;
    sc_in<sc_bv<32>> dataFromLastStage;
    sc_out<bool> requestToLastStage;
    sc_out<bool> rwToLastStage; // here can "last stage" be l2-cache
    sc_out<sc_bv<32>> addressToLastStage;
    sc_out<sc_bv<32>> outputToLastStage; // output forward to inside

    sc_out<bool> ready; // this component is ready to use
    sc_out<sc_bv<32>> outputData; // output to outside
    

    // we can possibly divide these bits into : Tag bits|Data bits|Control bits(dirty)
    sc_bv<156> *internal; // 28 tag bits + 128 data bits   the maximal space we need.
    
    SC_CTOR(CACHEL2);
    CACHEL2(sc_module_name name, int latency, int cacheLines,int cacheLineSize) : sc_module(name) {
        this->latency = latency;
        this->cacheLines = cacheLines;
        this->cacheLineSize = cacheLineSize;
        waitingForLatency = latency;
        hits = 0;
        miss = 0;
        offsetLength = (int)(log(cacheLineSize)/log(2));
        indexLength = (int)(log(cacheLines)/log(2));
        tagbits = 32 - offsetLength-indexLength;
        tagOffset = 32-tagbits;
        tagOffsetInCache = 156-tagbits;
        indexOffset = 32-tagbits-indexLength;
        cacheOffset = 32-indexLength+tagbits;
        dataOffset = 156-tagbits-(8*cacheLineSize);
        internal = new sc_bv<156>[cacheLines];

        this->name = (const char*)name;
        SC_THREAD(run);
        sensitive<<clk.pos()<<requestIncoming;
    }
    // as required the cache is full associative and shall be replaced with LRU
    void run(){
        while(true){
            wait();
        
        if(requestIncoming){ // upon request shall the component start to work   
            std::cout<<name<<" received request:"<< requestIncoming.read()<<std::endl;
            isWriteT = isWriteThrough.read(); 
            while(waitingForLatency<latency){
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
            std::cout<<name<<" waiting for request"<< std::endl;
            ready.write(true);
        }
        waitingForLatency = 0;
        }
    }

    int ifExist(int tag,int index){ // this method checks if the given address exist in the cache, returns the corresponding array index, and -1 if not exist
        int tag_tmp = internal[index%cacheLines].range(155,tagOffsetInCache).to_int();
        if (tag_tmp == tag)
            return index;
        return -1;
    }

    void write(){
        bool hit = true;
        int index = -1;
        int t_tmp = address.read().range(31,tagOffset).to_int();
        int i_tmp = address.read().range(tagOffset-1,indexOffset).to_int();
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

                sc_bv<32> addressToMem = address.read();
                addressToMem[0] = 0;
                addressToMem[1] = 0;
                std::cout<<name<<" aligned addr: "<< std::hex << std::setw(8) << std::setfill('0')<< addressToMem.to_int()<< std::endl;
                addressToLastStage.write(addressToMem); // address alignment with 4

                std::cout<<name<<" sended addr: "<< std::hex << std::setw(8) << std::setfill('0')<< addressToLastStage.read().to_int()<<" with original addr: "<< std::hex << std::setw(8) << std::setfill('0')<< address.read().to_int()<< std::endl;
                wait();
                requestToLastStage.write(false);
                while(!readyFromLastStage.read()){ // keep on waitin until last Stage is ready
                    wait();
                }
                std::cout<<name<<" with last stage ready for data preperation"<< std::endl;
                requestToLastStage.write(false);
                std::cout<<name<<" received data from last stage: "<< std::hex << std::setw(8) << std::setfill('0')<< dataFromLastStage.read().to_int()<< std::endl;
                index = address.read().range(tagOffset-1,indexOffset).to_int();
                internal[index%cacheLines].range(155,tagOffsetInCache) = address.read().range(31,tagOffset);
                internal[index%cacheLines].range(tagOffsetInCache-1,dataOffset) = dataFromLastStage.read();
            }
            if(hit&&!isWriteT){
                std::cout<<name<<" hit with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int() << std::endl;
                hits++;
            }
            // at this point, the required data is successfully loaded from last stage
            //calculate the offset and change the required data
            int offset = address.read().range(indexOffset-1,0).to_int();
            offset = offset & 0xfffffffc;
            int offset_input = 0;
            internal[index%cacheLines].range(tagOffsetInCache-1-offset*8,tagOffsetInCache-1-offset*8-31) = inputData.read().range(31,0);
            writeThrough();
            wait(); // wait for lastStage to exicute and change the request Singnal to false
            requestToLastStage.write(false);
        	ready.write(true);
            std::cout<<name<<" finished with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int() << std::endl;
    }
    void read(){
        bool hit = true;
        int index = -1;
        int t_tmp = address.read().range(31,tagOffset).to_int();
        int i_tmp = address.read().range(tagOffset-1,indexOffset).to_int();
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
                sc_bv<32> addressToMem = address.read();
                addressToMem[0] = 0;
                addressToMem[1] = 0;
                std::cout<<name<<" aligned addr: "<< std::hex << std::setw(8) << std::setfill('0')<< addressToMem.to_int()<< std::endl;
                addressToLastStage.write(addressToMem); // address alignment with 4
                std::cout<<name<<" sended addr: "<< std::hex << std::setw(8) << std::setfill('0')<< addressToLastStage.read().to_int()<<" with original addr: "<< std::hex << std::setw(8) << std::setfill('0')<< address.read().to_int()<< std::endl;
                wait();
                requestToLastStage.write(false);
                while(!readyFromLastStage.read()){ // keep on waitin until last Stage is ready
                    wait();
                }
                std::cout<<name<<" with last stage ready for data preperation"<< std::endl;
                requestToLastStage.write(false);
                std::cout<<name<<" received data from last stage: "<< std::hex << std::setw(8) << std::setfill('0')<< dataFromLastStage.read().to_int()<< std::endl;
                index = address.read().range(tagOffset-1,indexOffset).to_int();
                internal[index%cacheLines].range(155,tagOffsetInCache) = address.read().range(31,tagOffset);
                internal[index%cacheLines].range(tagOffsetInCache-1,dataOffset) = dataFromLastStage.read(); // here means the internal bus between caches are the same size with the cache line size
            }
            if(hit){
                std::cout<<name<<" hit with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int() << std::endl;
                hits++;
            }
            int offset = address.read().range(indexOffset-1,0).to_int();
            offset = offset & 0xfffffffc;
            sc_bv<32> output_tmp;
            output_tmp.range(31,0) = internal[index%cacheLines].range(tagOffsetInCache-1-offset*8,tagOffsetInCache-1-offset*8-31);
            outputData.write(output_tmp);
            ready.write(true);
            std::cout<<name<<" finished with addr: "<< std::hex << std::setw(8) << std::setfill('0')<<address.read().to_int() << std::endl;
    }
    void fetchFromLastStage();
    void writeThrough(){
        requestToLastStage.write(true);
        rwToLastStage.write(true);
        outputToLastStage.write(inputData.read());

        sc_bv<32> addressToMem = address.read();
        addressToMem[0] = 0;
        addressToMem[1] = 0;//address alignment

        addressToLastStage.write(addressToMem);
        wait();
        requestToLastStage.write(false);
    }
    ~CACHEL2(){
        delete[] internal;
    } 
};

