#include "definitions.h"
#include "stdlib.h"
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
/* unix only */
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/termios.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

void restore_input_buffering();
void handle_interrupt(int signal) {
  restore_input_buffering();
  printf("\n");
  exit(-2);
}
uint16_t sign_extend(uint16_t, int);
void update_flags(uint16_t);
int main(int argc, char *argv[]) {

  signal(SIGINT, handle_interrupt);
  disable_input_buffering();

  if (argc < 2) {
    printf("main [image-file]...\n\texpected image file");
    exit(2);
  }

  for (int i = 1; i < argc; ++i) {
    if (!read_image([argv[j])) {
      printf("failed to load image:%s\n", argv[i]);
      exit(1);
    }
  }

  reg[R_COND] = FL_ZRO;

  // starting position of the program
  enum { PC_START = 0x3000 };
  reg[R_PC] = PC_START;

  int running = 1;
  while (running) {
    uint16_t instr = mem_read(reg[R_PC]++);
    /*
     Extracting the opcode from the instrution, as remaning 4 bits after
     shifting usually is representation of an opcode.
    Register Addressing Mode:
       15            12 11   9 8   6  5  4  3 2    0
       +---------------+------+-----+---+----+------+
       |   OPCODE      |  DR  | SR1 | 0 | 00 | SR2  |
       +---------------+------------+---+----+------+
       SR1 -> first number to add
       SR2 -> second number to add
       DR -> destination register

    Immediate Addressing Mode:
       15            12 11   9 8   6  5  4         0
       +---------------+------+-----+---+-----------+
       |   OPCODE      |  DR  | SR1 | 1 |    imm5   |
       +---------------+------------+---+-----------+

       So, kinda like
       if bit[5] == 0 {
       DR = SR1 + SR2;
       } else if bit[5] == 1{
       DR =  SR1 + SEXT(imm5)
       }


      */
    uint16_t op = instr >> 12;

    switch (op) {
    case OP_ADD: {
      /* as register mode(using registers to pass around values) and immediate
      mode(using values directly ) both are supported
      the destination register as DR lies on the 9-11th position (15 MSB)*/
      uint16_t r0 = (instr >> 9) & 0x7;
      // first operand
      uint16_t r1 = (instr >> 6) & 0x7;
      // checking for immediate mode
      uint16_t imm_flag = (instr >> 5) & 0x1;

      if (imm_flag) {
        uint16_t imm5 = sign_extend(instr & 0x1F, 5); // 0x1F is 0001 1111
        reg[r0] = reg[r1] + imm5;
      } else {

        uint16_t r2 = instr & 0x7;
        reg[r0] = reg[r1] + reg[r2];
      }
      update_flags(r0);

      break;
    }

    case OP_BR: {
      break;
    }

    case OP_LD: {
      break;
    }

    case OP_ST: {
      break;
    }

    case OP_JSR: {
      break;
    }

    case OP_AND: {
      break;
    }

    case OP_LDR: {
      break;
    }

    case OP_STR: {
      break;
    }

    case OP_RTI: {
      break;
    }

    case OP_NOT: {
      break;
    }

    case OP_LDI: {
      break;
    }

    case OP_STI: {
      break;
    }

    case OP_JMP: {
      break;
    }

    case OP_RES: {
      break;
    }

    case OP_LEA: {
      break;
    }

    case OP_TRAP: {
      break;
    }
    }
  }

  restore_input_buffering();
  return EXIT_SUCCESS;
}

uint16_t sign_extend(uint16_t x, int bit_count) {
  // checking if the 7th bit is 1; and if it is, then
  if ((x >> (bit_count - 1)) & 1) {
    x |= (0xFFFF << bit_count);
  }
  return x;
}

// updating flags on the basis of result obtained when doing arithmetics
void update_flags(uint16_t r) {
  if (reg[r] == 0) {
    reg[R_COND] = FL_ZRO;
  } else if (reg[r] >> 15) {
    reg[R_COND] = FL_NEG;
  } else {
    reg[R_COND] = FL_POS;
  }
