#include <systemc>

SC_MODULE(MEMORY){

    sc_in<bool> rw;
    sc_in<sc_bv<32>> addr;
    sc_in<sc_bv<32>> rData;

    sc_out<sc_bv<32>> wData;
};