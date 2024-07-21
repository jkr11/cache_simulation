# C-Compiler
CC = gcc
# C++-Compiler
CXX = g++

# Compiler flags
CFLAGS = -Wall -Wextra -pedantic  -std=c17 
CXXFLAGS = -Wall -Wextra -pedantic  -std=c++14 -Iinclude -I$(SYSTEMC_HOME)/include

# Linker flags
LDFLAGS = -L$(SYSTEMC_HOME)/lib -lsystemc -lm -lstdc++

# Source and object files
CSRC = $(wildcard src/*.c)
CPPSRC = $(wildcard src/*.cpp)


COBJS = $(CSRC:.c=.o)
CPPOBJS = $(CPPSRC:.cpp=.o)


# Executables
EXEC = project



all: debug

debug: CFLAGS += -g
debug: CXXFLAGS += -g
debug: $(EXEC)

# -Wno-aggressive-loop-optimizations in memory.cpp at to_uint of sc_bv<8> DO NOT USE
# -O1 works fine
release: CFLAGS += -O2
release: CXXFLAGS += -O2
release: $(EXEC)

$(EXEC): $(COBJS) $(CPPOBJS)
	$(CXX) $(COBJS) $(CPPOBJS) $(LDFLAGS) -o $@



# Compile C src
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile C++ src
src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:

	rm -f $(COBJS) $(CPPOBJS) $(EXEC) 
