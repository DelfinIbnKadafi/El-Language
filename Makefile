# El Language Build System

CC = gcc

CFLAGS = -Wall -Wextra -std=c99
LDLIBS = -lm

SRC = src

BUILD = compiler

TARGET = $(BUILD)/elvm

FILES = \
  $(SRC)/main.c \
  $(SRC)/parser.c \
  $(SRC)/lexer.c \
  $(SRC)/elvm.c


all: $(TARGET)

$(TARGET): $(FILES)
	# Create output directory
	mkdir -p $(BUILD)

	# Compile El VM
	$(CC) $(CFLAGS) $(FILES) -o $(TARGET) $(LDLIBS)


clean:
	# Remove compiled files
	rm -rf $(BUILD)/elvm


rebuild: clean all
