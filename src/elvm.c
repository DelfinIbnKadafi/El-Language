#include <stdio.h>
#include "elvm.h"

void VMRun(Instruction* code, int count) {
  // Instruction pointer
  int ip = 0;

  while(ip < count) {
    // Fetch current instruction
    Instruction instruction = code[ip++];

    switch(instruction.opcode) {
      case OP_PRINT:
        // Print string output
        printf("%s\n", instruction.text);
        break;

      case OP_HALT:
        // Stop VM
        return;
    }
  }
}