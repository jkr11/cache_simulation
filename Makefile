# C-Compiler
CC = gcc
#C++-Compiler
CXX = g++

# Path to systemC installation (how do we build this?)
SCPATH = ../systemc

#additional compiler flags
CFLAGS = -Wall -Wextra -pedantic -std=c17 
CXXFLAGS = -Wall -Wextra -pedantic -std=c++17


INCLUDE_DIR = include
SRC_DIR = src
TEST_DIR = test

SRCS = $(TEST_DIR)/test_parse_csv.c $(SRC_DIR)/csv_parser.c
OBJS = $(SRCS:.c=.o)
EXEC = $(TEST_DIR)/test_parse_csv

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(EXEC)

$(SRC_DIR)/%.0: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

clean:
	rm -f $(OBJS) $(EXEC)
