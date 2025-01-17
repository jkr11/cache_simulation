🌍 **Available languages**:
- 🇬🇧 [English](README.md)
- 🇩🇪 [Deutsch](README.de.md)

# GRA TEAM 192

## Assignment 

The goal of the project was to implement a two-level direct associative cache using SystemC, C++, and a C framework.

The cache should operate in two levels (L1 and L2) and simulate a memory hierarchy.

## Implementation


Representation of a cache line |tag|data|empty| as a struct
```C++
struct CacheLine {
  int tag;
  uint8_t *bytes;  
  int empty;
};
```

Implementation of three SystemC modules:

```C++
SC_MODULE(CACHEL1)
SC_MODULE(CACHEL2)
SC_MODULE(MEMORY)
```
each with a storage
```C++
Cachelines* internal;
```
 and
```C++
std::unordered_map<uint32_t,uint8_t>  internal;
```
The project utilizes unaligned memory access.

### Calculating the number of hits and misses, cycles

The total number of hits and misses is the sum of the counters in L1 and L2.

For cross-row accesses, the L1 cache latency is counted twice, as the implementation splits each access into two.

Cycle counting occurs in run_simulation, while latencies are handled using wait(2 * latency, SC_NS) in each of the three modules.

## Literature review

The literature research revealed that the standard latencies are L1 = 4 cycles, L2 = 16 cycles and memory = 400 cycles. Furthermore, unaligned memory access is used in standard implementations. Directly associative caches enable simple implementation of access and management. Write-through simplifies implementation and coherence.

## Methodology and measurement environment

The simulation was performed in SystemC, small examples were analyzed and verified using gdb and GTKWave (also for latency at cycle level), large examples via .csv files.

The correctness of read-in/read-out by cache was verified by comparison with the results from the tester that communicates directly with memory.

The access .csv files are either in test.ipynb or generated by hand, tested and analyzed using .sh and .ipynb.

The calculation of the number of gates is comparable to the circuit design in (circuit/).

## Project results

The project successfully validates the expected cache behavior described in the literature.

## Contribution

The commit graph can be found in gitgraph.

### Xuanqi Meng

Development of one version of three modules and the simulation, which was later chosen by other team members as the submission template.

Partial debugging and logic optimization (module synchronization and hit/miss counter).

Design of the circuit of the corresponding program logic and correctness check in slides.

### Jeremias Rieser 

Creation of tests and access .csv files, verification and analysis of the results. Debugging and partially solving memory safety issues.

Implementation of the framework program, Makefiles, as well as inclusion of literature research, slides and graphics.

Development of a cache simulation used only for structure development.

### Artem Bilovol

Development of a version of three modules and the simulation (not selected as a template to be delivered)

Creating tests for the correct functionality of the cache (tests/)

Debugging, especially handling file paths and edge cases

Summary and outlook of slides
