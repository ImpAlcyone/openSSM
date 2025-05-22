#
#    **                                                **
#   ******************************************************
#  ***  THIS PROGRAM IS THE PROPERTY OF ALCYONE LIMITED ***
#   ******************************************************
#    **                                                **
#
#  Program suite for Subaru ECU interface.
#  Copyright (C) Alcyone Limited 2007.
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#

# Compiler and flags
CC := gcc
CFLAGS := -Iinclude -Wall -Wextra -O2
LDFLAGS :=

# Directories
SRC_DIR := src
OBJ_DIR := $(SRC_DIR)/obj
BIN_DIR := bin
INC_DIR := include

# Create required directories
$(shell mkdir -p $(OBJ_DIR) $(BIN_DIR))

# Source files
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(filter-out $(SRC_DIR)/tcuscan.c $(SRC_DIR)/ecuscan.c,$(SRCS)))

# Object files with -lcurses
OBJS_CURSES := $(OBJ_DIR)/tcuscan.o $(OBJ_DIR)/ecuscan.o

# Executables
TARGETS := tcuscan ecuscan ecudump tcudump 4wsdump checkecu checktcu 
ALL_TARGETS := $(addprefix $(BIN_DIR)/, $(TARGETS))

# Default target
ALL: $(ALL_TARGETS)

# Executable build rules
$(BIN_DIR)/tcuscan: $(OBJ_DIR)/tcuscan.o $(OBJ_DIR)/ssm.o
	$(CC) $^ -lcurses -o $@

$(BIN_DIR)/ecuscan: $(OBJ_DIR)/ecuscan.o $(OBJ_DIR)/ssm.o
	$(CC) $^ -lcurses -o $@

$(BIN_DIR)/ecudump: $(OBJ_DIR)/ecudump.o $(OBJ_DIR)/ssm.o
	$(CC) $^ -o $@

$(BIN_DIR)/tcudump: $(OBJ_DIR)/tcudump.o $(OBJ_DIR)/ssm.o
	$(CC) $^ -o $@

$(BIN_DIR)/4wsdump: $(OBJ_DIR)/4wsdump.o $(OBJ_DIR)/ssm.o
	$(CC) $^ -o $@

$(BIN_DIR)/checkecu: $(OBJ_DIR)/checkecu.o
	$(CC) $^ -o $@

$(BIN_DIR)/checktcu: $(OBJ_DIR)/checktcu.o
	$(CC) $^ -o $@


# Compile object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(INC_DIR)/ssm.h
	$(CC) $(CFLAGS) -c $< -o $@

# Debug build (overrides optimization)
debug: CFLAGS := -Iinclude -Wall -Wextra -g
debug: clean ALL

# Clean up
clean:
	find $(OBJ_DIR) -name '*.o' -delete
	find $(BIN_DIR) -maxdepth 1 -type f -exec rm -f {} +

.PHONY: ALL clean debug
