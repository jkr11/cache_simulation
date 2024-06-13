#include <systemc>
#include <systemc.h>
#include <map>

SC_MODULE(MEMORY)
{
    sc_in<bool> rw;
    sc_in<sc_bv<32>> addr;
    sc_in<sc_bv<32>> rData;

    sc_out<sc_bv<32>> wData;

    std::map<uint32_t, uint8_t> coreImage;
    unsigned int latency;

    sc_in<bool> clk;

    SC_CTOR(MEMORY)
    {
        SC_THREAD(behaviour);
        sensitive << rw << addr;
    }

    MEMORY(sc_module_name name, unsigned int memLat): sc_module(name)
    {
        latency = memLat;
    }

    uint8_t getOrZero(std::map<uint32_t, uint8_t> coreImage, uint32_t address)
    {
        auto data = coreImage.find(address);
        if (data != coreImage.end())
        {
            return data->second;
        }
        return 0;
    }

    void behaviour()
    {
        while (true)
        {
            // Getting adress
            uint32_t address = addr->read().to_uint();

            if (rw->read())
            {
                // Writing
                uint32_t data = rData->read().to_uint();

                coreImage[address] = static_cast<uint8_t>(data & 0xFF);
                coreImage[address + 1] = static_cast<uint8_t>((data >> 8) & 0xFF);
                coreImage[address + 2] = static_cast<uint8_t>((data >> 16) & 0xFF);
                coreImage[address + 3] = static_cast<uint8_t>((data >> 24) & 0xFF);
            }
            else
            {
                // Reading
                uint32_t data = 0;

                data |=
                    static_cast<uint32_t>(getOrZero(coreImage, address));
                data |=
                    static_cast<uint32_t>(getOrZero(coreImage, address + 1)) << 8;
                data |=
                    static_cast<uint32_t>(getOrZero(coreImage, address + 2)) << 16;
                data |=
                    static_cast<uint32_t>(getOrZero(coreImage, address + 3)) << 24;

                wData->write(data);
            }

            // Memory latency (waiting for actions in RAM IRL)
            for (unsigned int i = 0; i < latency; i++)
            {
                next_trigger(clk.posedge_event());
            }
        }
    }
};
