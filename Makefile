SCPATH = ../systemc
# C-Compiler
CC = gcc
# C++-Compiler
CXX = g++

# Compiler flags
CFLAGS = -Wall -Wextra -pedantic -g -std=c17 
CXXFLAGS = -Wall -Wextra -pedantic -g -std=c++14 -Iinclude -I$(SCPATH)/include

# Linker flags
LDFLAGS = -L$(SCPATH)/lib -lsystemc -lm -lstdc++

# Source and object files
CSRC = $(wildcard src/*.c)
CPPSRC = $(wildcard src/*.cpp)

COBJS = $(CSRC:.c=.o)
CPPOBJS = $(CPPSRC:.cpp=.o)

# Executables
EXEC = project
#TEST_EXEC = test_project

all: $(EXEC)

$(EXEC): $(COBJS) $(CPPOBJS)
	$(CXX) $(COBJS) $(CPPOBJS) $(LDFLAGS) -o $@

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

src/%.o: src/%.

clean:
	rm -f $(COBJS) $(CPPOBJS) $(EXEC) 