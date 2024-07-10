#include <systemc>
#include <systemc.h>

SC_MODULE(MEMORY){ // due to the defination of Request, I assume that the max address and maximal length of Data are 4 Bytes (uint32)
    int latency;
    sc_in<bool> clk;

    int latencyWaited = 0;
    sc_in<bool> requestIncoming;
    sc_in<bool> rw;
    sc_in<sc_bv<32>> addr;
    sc_in<sc_bv<32>> rData;

    sc_out<bool> ready;
    sc_out<sc_bv<32>> wData;
    
    std::unordered_map<uint32_t,uint8_t>  internal; // this unordered_map allows the small range of simulation of 4GB memory in PC

    SC_CTOR(MEMORY);
    MEMORY(sc_module_name name,int latency): sc_module(name){ 
        this->latency = latency;
        this->latencyWaited = latency;
        SC_THREAD(run);
        sensitive<<clk.pos()<<requestIncoming;
    }
    void run(){
        while(true){
            wait();
            if(requestIncoming.read()){
                while (latencyWaited < latency){
                    //std::cout<<"memory waiting for latency"<<std::endl;
                    latencyWaited++;
                    ready.write(false);
                    wait();
                }
                std::cout<<"memory request incomming From L2"<<std::endl;
                uint32_t address = stoi(addr.read().to_string(),0,2);
                if(rw.read()){ // true for write
                    for(int i = 0; i< 4; i++){
                        internal[address+i] = (uint8_t)rData.read().range((i+1)*8-1,i).to_int();
                        std::cout<< "written into address: "<< address+i<<" with data: "<<  (uint8_t)rData.read().range((i+1)*8-1,i).to_int()<<std::endl;
                    }
                }else{ // false for read
                    sc_bv<32> data;
                    for(int i = 0; i< 4;i++){
                        if (internal.find(address+i)== internal.end()){
                            data.range((i+1)*8-1,i) = 0x00;
                            std::cout<< "read from address: "<< address+i<<" with data: 0x00"<<std::endl;
                        }
                        else{
                            data.range((i+1)*8-1,i) = internal[address+i];
                            std::cout<< "read from address: "<< address+i<<" with data: "<<internal[address+i]<<std::endl;
                            }
                    }
                    wData.write(data);
                    ready.write(true);
                    latencyWaited = 0;
                    std::cout<<"memory finished current operation"<<std::endl;
                }
            }else{
                std::cout<<"memory waiting for request"<<std::endl;
                ready.write(true);
            }
        }
    }
     ~MEMORY(){}
};