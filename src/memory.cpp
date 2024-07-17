#ifndef MEMORY_CPP
#define MEMORY_CPP
#include <iostream>
#include <iomanip>
#include <systemc>
#include <systemc.h>
#include <unordered_map>

#define MEMLOG
#define MEMLOG_TIME
SC_MODULE(MEMORY){ // due to the defination of Request, I assume that the max address and maximal length of Data are 4 Bytes (uint32)
    int latency;
    sc_in<bool> clk;

    int latencyWaited = 0; // the latency counter
    sc_in<bool> requestIncoming;
    sc_in<bool> rw;
    sc_in<sc_bv<32>> addr;
    sc_in<sc_bv<32>> rData; // the incomming data from L2

    sc_out<bool> ready;
    sc_out<sc_bv<32>> wData; // output data to L2
    
    std::unordered_map<uint32_t,uint8_t>  internal; // this unordered_map allows the small range of simulation of 4GB memory in PC

    SC_CTOR(MEMORY);
    MEMORY(sc_module_name name,int latency): sc_module(name){ 
        this->latency = latency;
        this->latencyWaited = 0;
        SC_THREAD(run);
        sensitive<<clk.pos()<<requestIncoming;
    }
    void run(){
        while(true){
            wait();
            if(requestIncoming.read()){
                #ifdef MEMLOG_TIME
                std::cout<<"memory request incomming From L2 at time: "<<sc_time_stamp()<<std::endl;
                #endif
                ready.write(false);
                /*while (latencyWaited < latency){ // first count the latency then execute the funtionality
                    //std::cout<<"memory waiting for latency"<<std::endl;
                    latencyWaited++;
                    ready.write(false);
                    wait();
                }*/
                wait(2*latency,SC_NS);
                
                uint32_t address = addr.read().to_uint();
                if(rw.read()){ // true for write
                    #ifdef MEMLOG
                    std::cout<< "memory recieved data in binary: "<< rData.read().to_string()<<std::endl;
                    #endif
                    for(int i = 0; i< 4; i++){
                        internal[address+i] = rData.read().range((i+1)*8-1,i*8).to_uint();
                        #ifdef MEMLOG
                        std::cout<< "written into address: "<< address+i<<" with data: "<< std::hex << std::setw(1) << std::setfill('0')<< rData.read().range((i+1)*8-1,i*8).to_uint()<<std::endl;
                        #endif
                    }
                }else{ // false for read
                    sc_bv<32> data;
                    for(int i = 0; i< 4;i++){
                        if (internal.find(address+i)== internal.end()){
                            data.range((i+1)*8-1,i*8) = 0x00;
                            #ifdef MEMLOG
                            std::cout<< "read from address: "<< address+i<<" with data: 0x00"<<std::endl;
                            #endif
                        }
                        else{
                            data.range((i+1)*8-1,i*8) = internal[address+i];
                            #ifdef MEMLOG
                            std::cout<< "read from address: "<< address+i<<" with data: "<< std::hex << std::setw(2) << std::setfill('0')<<(int)internal[address+i]<<std::endl;
                            #endif
                            }

                    }
                    wData.write(data);
                }
                ready.write(true);
                #ifdef MEMLOG_TIME
                std::cout<<"memory finished current operation at time: "<<sc_time_stamp()<<std::endl;
                #endif
                //latencyWaited = 0;
            }else{
                //std::cout<<"memory waiting for request"<<std::endl;
                ready.write(true);
            }
        }
    }
     ~MEMORY(){}
};

#endif