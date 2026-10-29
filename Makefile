# El Language Build System

CC = gcc

CFLAGS = -Wall -Wextra -std=c99
LDLIBS = -lm

# Loading plugins needs the dynamic loader on Linux (it is part of the C
# library on Windows and macOS)
ifneq ($(OS),Windows_NT)
ifneq ($(shell uname -s),Darwin)
LDLIBS += -ldl
endif
endif

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
  $(SRC)/elvm.c \
  $(SRC)/source.c \
  $(SRC)/json.c \
  $(SRC)/plugin.c


.PHONY: all clean rebuild

all: $(TARGET)

$(TARGET): $(FILES) | $(BUILD)
	$(CC) $(CFLAGS) $(FILES) -o $(TARGET) $(LDLIBS)

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -f $(BUILD)/elvm $(BUILD)/elvm.exe


rebuild: clean all
