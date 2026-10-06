# El Language Build System

CC = gcc

CFLAGS = -Wall -Wextra -std=c99
LDLIBS = -lm

SRC = src

BUILD = compiler

ifeq ($(OS),Windows_NT)
TARGET = $(BUILD)/elvm.exe
else
TARGET = $(BUILD)/elvm
endif

FILES = \
  $(SRC)/main.c \
  $(SRC)/parser.c \
  $(SRC)/lexer.c \
  $(SRC)/elvm.c


.PHONY: all clean rebuild

all: $(TARGET)

$(TARGET): $(FILES) | $(BUILD)
	$(CC) $(CFLAGS) $(FILES) -o $(TARGET) $(LDLIBS)

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -f $(BUILD)/elvm $(BUILD)/elvm.exe


rebuild: clean all
