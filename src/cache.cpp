#include <systemc>
#include <systemc.h>

SC_MODULE(CACHE)
{
    unsigned int latency;
    unsigned int cacheLines;
    unsigned int cacheLineSize;

    unsigned int misses;
    unsigned int hits;

    sc_in<bool> rw;
    sc_in<sc_bv<32>> addr;
    sc_in<sc_bv<32>> rData;

    sc_out<sc_bv<32>> wData;

    sc_out<bool> rwToLastStage;
    sc_out<sc_bv<32>> addrToLastStage;
    sc_in<sc_bv<32>> rDataFromLastStage;

    sc_out<sc_bv<32>> wDataToLastStage;

    sc_in<bool> clk;

    // Attributes required for the implementation
    unsigned int offsetBits = 0;
    unsigned int indexBits = 0;
    unsigned int cacheLinesRoundedToPowerOfTwo = 1;
    unsigned int cutOff = 0;
    uint32_t address;
    uint32_t data;

    // Additional attributes for "help"-functions
    uint32_t currentLines[2];
    uint32_t lineNumber;
    uint8_t* storage;
    uint32_t* indexes;
    uint32_t* tags;
    bool isItAHit = true;

    SC_CTOR(CACHE)
    {
        SC_THREAD(behaviour);
        sensitive << rw << addr;
    }

    CACHE(sc_module_name name,
          unsigned int cacheLat,
          unsigned int thisCacheLines,
          unsigned int thisCacheLinesSize
    ): sc_module(name)
    {
        latency = cacheLat;
        cacheLines = thisCacheLines;
        cacheLineSize = thisCacheLinesSize;
    }

    // A function to renew all data in a cacheline from the next stage
    void line_renewer(uint32_t lineNumber, uint32_t addr)
    {
        // Turning offsetbits into zeros
        addr = (address >> offsetBits) << offsetBits;

        for (unsigned int i = 0; i < cacheLineSize; i++)
        {
            addrToLastStage->write(addr);
            rwToLastStage->write(false);


            *(storage + lineNumber + i) = rDataFromLastStage.read().to_uint();
        }

        // Updating tags
        *(tags + lineNumber) = addr >> offsetBits;

        // It is a miss
        isItAHit = false;
    }

    // A function to check and renew all cachelines required for the adress,
    // and then get required data
    void reader()
    {
        uint32_t data = 0;
        currentLines[0] = indexes[address >> offsetBits % indexBits];
        currentLines[1] = indexes[(address + 3) >> offsetBits % indexBits];

        if (tags[currentLines[0]] != address >> cutOff)
        {
            line_renewer(currentLines[0], address);
        }

        unsigned int k = 0;
        unsigned int i = address % cacheLineSize;

        // Reading the first cacheline until the end
        while (i < cacheLineSize)
        {
            data |=
                static_cast<uint32_t>(*(storage + currentLines[0] * cacheLineSize + i)) << 8 * k;
            i++;
            k++;
        }
        if (k == 4) { return; }

        if (tags[currentLines[1]] != (address + 3) >> cutOff)
        {
            line_renewer(currentLines[1], address + 3);
        }

        // Reading the second cacheline until all data received
        i = 0;
        while (k < 4)
        {
            data |=
                static_cast<uint32_t>(*(storage + currentLines[0] * cacheLineSize + i)) << 8 * k;
            i++;
            k++;
        }
    }

    void writer()
    {
        uint32_t data = wData.read().to_uint();
        currentLines[0] = indexes[address >> offsetBits % indexBits];
        currentLines[1] = indexes[(address + 3) >> offsetBits % indexBits];

        // Renewing cachelines from the next stage
        if (tags[currentLines[0]] != address >> cutOff)
        {
            line_renewer(currentLines[0], address);
        }
        if (tags[currentLines[1]] != (address + 3) >> cutOff)
        {
            line_renewer(currentLines[1], address + 3);
        }

        // Write-Through to the next stage
        addrToLastStage->write(address);
        wDataToLastStage->write(data);
        rwToLastStage->write(true);

        // Renewing data in this cache

        // Breaking the data in 4 bytes and write every one of them in its place
        uint8_t data0 = data;
        uint8_t data1 = data >> 8;
        uint8_t data2 = data >> 16;
        uint8_t data3 = data >> 24;

        // If the number of cachelines is 2 ^ n - 1, it is possible that the data for the second line overwrites the first
        // In this case the data is renewed only for the second line
        if (cacheLinesRoundedToPowerOfTwo - cacheLines == 1 &&
            currentLines[1] == 0 &&
            tags[currentLines[0]] != address >> cutOff)
        {
            if (tags[currentLines[1]] == (address + 1) >> cutOff)
            {
                *(storage + (cacheLineSize * currentLines[0] + (address / offsetBits) + 1) / (cacheLineSize *
                    cacheLines)) = data1;
            }
            if (tags[currentLines[1]] == (address + 2) >> cutOff)
            {
                *(storage + (cacheLineSize * currentLines[0] + (address / offsetBits) + 2) / (cacheLineSize *
                    cacheLines)) = data2;
            }

            *(storage + (cacheLineSize * currentLines[0] + (address / offsetBits) + 3) / (cacheLineSize *
                cacheLines)) = data3;

            return;
        }

        *(storage + (cacheLineSize * currentLines[0] + (address / offsetBits)) / (cacheLineSize * cacheLines)) =
            data0;
        *(storage + (cacheLineSize * currentLines[0] + (address / offsetBits) + 1) / (cacheLineSize *
                cacheLines)) =
            data1;
        *(storage + (cacheLineSize * currentLines[0] + (address / offsetBits) + 2) / (cacheLineSize *
                cacheLines)) =
            data2;
        *(storage + (cacheLineSize * currentLines[0] + (address / offsetBits) + 3) / (cacheLineSize *
                cacheLines)) =
            data3;
    }

    void behaviour()
    {
        // To use all given cachelines a second level bitmask is used
        // If cacheLines is not a power of two,
        // the same bitmask is used for two cachelines with an additional bitmask

        // Calculating the number of offset bits
        for (unsigned int i = cacheLineSize; i > 1; i /= 2)
        {
            offsetBits++;
        }

        // Calculating number of index bits and their power of 2
        for (; cacheLinesRoundedToPowerOfTwo < cacheLines; cacheLinesRoundedToPowerOfTwo *= 2)
        {
            indexBits++;
        }

        // Creating an index bit table for the fast access
        indexes = new uint32_t[cacheLinesRoundedToPowerOfTwo];
        for (unsigned int i = 0; i < cacheLinesRoundedToPowerOfTwo; i++)
        {
            indexes[i] = i % (cacheLines - 1);
        }

        // Calculating cutoff from an adress to create a tag
        cutOff = offsetBits + indexBits;
        if (cacheLinesRoundedToPowerOfTwo != cacheLines)
        {
            cutOff--;
        }

        // Cachelines with the second level bitmask are going
        // to be concentrated at the ends of the cache (the same number at the start and at the end)
        // With the previous information the cache is to be formed
        storage = new uint8_t[cacheLines * cacheLineSize];

        // To not overcomplicate the design the tag of the length of 32 bits is used
        // Excessive bits to be ignored and initiated to zero
        tags = new uint32_t[cacheLines];
        for (unsigned int i = 0; i < cacheLines; i++)
        {
            tags[i] = 0xFFFFFFFF;
        }

        // Simulation of the cache working
        while (true)
        {
            // Getting adress
            address = addr->read().to_uint();

            if (rw->read())
            {
                // Writing
                writer();
            }
            else
            {
                // Reading
                reader();

                // Sending data back
                wData.write(data);
            }

            // Counting a hit/ miss
            if (isItAHit)
            {
                hits++;
                isItAHit = true;
            }
            else misses++;

            // Latency (waiting for actions IRL)
            for (unsigned int i = 0; i < latency; i++)
            {
                next_trigger(clk.posedge_event());
            }
        }
    }
};
