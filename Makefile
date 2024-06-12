ifndef SYSTEMC_HOME
SYSTEMC_HOME = /home/jkr/uni/sem4/gra/workspace/systemc  # choose your own one here if the path does not work
endif



# C-Compiler
CC = gcc
#C++-Compiler
CXX = g++

# Path to systemC installation (how do we build this?)
SCPATH = ../systemc

#additional compiler flags  // -Werror for final
CFLAGS = -Wall -Wextra -pedantic -g -std=c17 
CXXFLAGS = -Wall -Wextra -pedantic -g -std=c++14

SYSTEMC_HOME = 

INCLUDES = -Iinclude -I$(SYSTEMC_HOME)/include -lm

CSRC = $(wildcard src/*.c)
CPPSRC = $(wildcard src/*.cpp)
TESTSRC = $(wildcard test/*.c)

COBJS = $(CSRC:.c=.o)
CPPOBJS = $(CPPSRC:.cpp=.o)
TESTOBJS = $(TESTSRC:.c=.o)

# these are the actual executables
EXEC = project
TEST_EXEC = test_project

# Library paths (systemc) and libraries

LDFLAGS = -L$(SYSTEMC_HOME)/lib-linux64
LIBS = -lsystemc


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