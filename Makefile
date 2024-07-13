# C-Compiler
CC = gcc
# C++-Compiler
CXX = g++

# Compiler flags
CFLAGS = -Wall -Wextra -pedantic -g -std=c17 
CXXFLAGS = -Wall -Wextra -pedantic -g -std=c++14 -Iinclude -I$(SYSTEMC_HOME)/include

# Linker flags
LDFLAGS = -L$(SYSTEMC_HOME)/lib -lsystemc -lm -lstdc++

# Source and object files
CSRC = $(wildcard src/*.c)
CPPSRC = $(wildcard src/*.cpp)
TESTSRC = $(wildcard test/*.c)

COBJS = $(CSRC:.c=.o)
CPPOBJS = $(CPPSRC:.cpp=.o)
TESTOBJS = $(TESTSRC:.c=.o)

# Executables
EXEC = project
#TEST_EXEC = test_project

all: $(EXEC) $(TEST_EXEC)

$(EXEC): $(COBJS) $(CPPOBJS)
	$(CXX) $(COBJS) $(CPPOBJS) $(LDFLAGS) -o $@

#$(TEST_EXEC): $(TESTOBJS) $(COBJS) $(CPPOBJS)
#$(CXX) $(TESTOBJS) $(COBJS) $(CPPOBJS) $(LDFLAGS) -o $@

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

src/%.o: src/%.

clean:
	rm -f $(COBJS) $(CPPOBJS) $(EXEC) $(TEST_EXEC)