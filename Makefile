ifndef SYSTEMC_HOME
SYSTEMC_HOME = /home/jkr/uni/sem4/gra/workspace/systemc
endif



# C-Compiler
CC = gcc
#C++-Compiler
CXX = g++

# Path to systemC installation (how do we build this?)
SCPATH = ../systemc

#additional compiler flags
CFLAGS = -Wall -Wextra -pedantic -g -std=c17 
CXXFLAGS = -Wall -Wextra -pedantic -g -std=c++17

SYSTEMC_HOME = 

INCLUDES = -Iinclude -I$(SYSTEMC_HOME)/include

CSRC = $(wildcard src/*.c)
CPPSRC = $(wildcard src/*.cpp)
TESTSRC = $(wildcard test/*.c)

COBJS = $(CSRC:.c=.o)
CPPOBJS = $(CPPSRC:.cpp=.o)
TESTOBJS = $(TESTSRC:.c=.o)

EXEC = project
TEST_EXEC = test_project

all: $(EXEC) $(TEST_EXEC)

$(EXEC): $(COBJS) $(CPPOBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

$(TEST_EXEC): $(TESTOBJS) $(COBJS) $(CPPOBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

src/%.o: src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

test/%.o: src/%.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(COBJS) $(CPPOBJS) $(TESTOBJS)

source: $(EXEC)

tests: $(TEST_EXEC)

run: $(EXEC)
	./$(EXEC)

test: $(TEST_EXEC)
	./$(TEST_EXEC)

.PHONY: all clean run test