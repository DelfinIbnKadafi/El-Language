# El Language Build System

CC = gcc

CFLAGS = -Wall -Wextra -std=c99

SRC = src

BUILD = compiler

TARGET = $(BUILD)/elvm

FILES = \
  $(SRC)/main.c \
  $(SRC)/lexer.c \
  $(SRC)/elvm.c


all: $(TARGET)

$(TARGET): $(FILES)
	# Create output directory
	mkdir -p $(BUILD)

	# Compile El VM
	$(CC) $(CFLAGS) $(FILES) -o $(TARGET)


clean:
	# Remove compiled files
	rm -rf $(BUILD)/elvm


rebuild: clean all