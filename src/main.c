#include "definitions.h"
#include "stdlib.h"
// #include <locale>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
/* unix only */
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

uint16_t check_key(void);

uint16_t swap16(uint16_t);

uint16_t mem_read(uint16_t);

// reading from memory
uint16_t mem_read(uint16_t address) {
  if (address == MR_KBSR) {
    if (check_key()) {
      memory[MR_KBSR] = 1 << 15;
      memory[MR_KBDR] = getchar();
    } else {
      memory[MR_KBSR] = 0;
    }
  }
  return memory[address];
}
void mem_write(uint16_t, uint16_t);

void mem_write(uint16_t address, uint16_t value) {
  memory[address] = value;
} // assigning values

uint16_t read_image(char *);

void disable_input_buffering();

int main(int argc, char *argv[]) {

  signal(SIGINT, handle_interrupt);
  disable_input_buffering();

  if (argc < 2) {
    printf("main [image-file]...\n\texpected image file");
    exit(2);
  }

  for (int i = 1; i < argc; ++i) {
    if (!read_image(argv[i])) {
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

      uint16_t PCoffset9 =
          sign_extend(instr & 0x1FF, 9); // checking if negative value

      uint16_t n_z_p_flag = (instr >> 9) & 0x7;
      if (n_z_p_flag & reg[R_COND]) {
        reg[R_PC] += PCoffset9;
      }
      break;
    }

    case OP_LD: {
      uint16_t PCoffset9 = sign_extend(instr & 0x1FF, 9);
      uint16_t r0 = (instr >> 9) & 0x7;
      reg[r0] = mem_read(reg[R_PC] + PCoffset9);
      update_flags(r0);

      break;
    }

    case OP_ST: {
      uint16_t PCoffset9 = sign_extend(instr & 0x1FF, 9);
      uint16_t r0 = (instr >> 9) & 0x7;
      // mem_read(reg[R_PC] + PCoffset9) = r0;

      mem_write(reg[R_PC] + PCoffset9, reg[r0]); // assigning value to SR
      break;
    }

    case OP_JSR: {
      reg[R_R7] = reg[R_PC];
      uint16_t flag = (instr >> 11) & 1;
      if (flag) {
        uint16_t pc_offset = sign_extend(instr & 0x7FF, 11);
        reg[R_PC] += pc_offset; // JSR
      } else {
        uint16_t baseR = (instr >> 6) & 0x7;
        reg[R_PC] = reg[baseR]; // JSRR
      }
      break;
    }

    case OP_AND: {
      uint16_t r0 = (instr >> 9) & 0x7;
      // first operand
      uint16_t r1 = (instr >> 6) & 0x7;
      // checking for immediate mode
      uint16_t imm_flag = (instr >> 5) & 0x1;

      if (imm_flag) {
        uint16_t imm5 = sign_extend(instr & 0x1F, 5); // 0x1F is 0001 1111
        reg[r0] = reg[r1] & imm5;
      } else {
        uint16_t r2 = instr & 0x7;
        reg[r0] = reg[r1] & reg[r2];
      }
      update_flags(r0);
      break;
    }

    case OP_LDR: {
      uint16_t offset6 = sign_extend(instr & 0x3F, 6);
      uint16_t r0 = (instr >> 6) & 0x7; // baseR
      uint16_t r1 = (instr >> 9) & 0x7; // DR

      reg[r1] = mem_read(reg[r0] + offset6);
      update_flags(r1);
      break;
    }

    case OP_STR: {
      uint16_t offset6 = sign_extend(instr & 0x3F, 6);
      uint16_t r0 = (instr >> 6) & 0x7; // baseR
      uint16_t r1 = (instr >> 9) & 0x7; // SR
      // mem[BaseR + SEXT(offset6)] = SR

      mem_write(reg[r0] + offset6, reg[r1]);
      break;
    }

    case OP_NOT: {
      uint16_t r0 = (instr >> 6) & 0x7; // source register
      uint16_t r1 = (instr >> 9) & 0x7; // destination register

      reg[r1] = ~reg[r0];
      update_flags(r1);
      break;
    }

    case OP_LDI: {
      uint16_t PCoffset9 = sign_extend(instr & 0x1FF, 9);
      uint16_t r0 = (instr >> 9) & 0x7;
      reg[r0] = mem_read(mem_read(reg[R_PC] + PCoffset9));
      update_flags(r0);
      break;
    }

    case OP_STI: {
      uint16_t r0 = (instr >> 9) & 0x7;
      uint16_t pc_offset = sign_extend(instr & 0x1FF, 9);
      mem_write(mem_read(reg[R_PC] + pc_offset), reg[r0]);

      break;
    }

    case OP_JMP: {
      uint16_t r1 = (instr >> 6) & 0x7;
      // program counter now has next instruction to be jumped on to
      reg[R_PC] = reg[r1];
      break;
    }

    case OP_LEA: {
      uint16_t PCoffset9 = sign_extend(instr & 0x1FF, 9);
      uint16_t r0 = (instr >> 9) & 0x7;
      reg[r0] = reg[R_PC] + PCoffset9;
      update_flags(r0);
      break;
    }

    case OP_TRAP: {
      uint16_t trapvect8 = (instr & 0xFF) | 0x0;
      reg[R_R7] = reg[R_PC];
      switch (instr & 0xFF) {

      case TRAP_GETC:
        reg[R_R0] = (uint16_t)getchar();
        update_flags(R_R0);
        break;

      case TRAP_OUT:
        putc((char)reg[R_R0], stdout);
        fflush(stdout);
        break;

      case TRAP_PUTS: {
        uint16_t *c = memory + reg[R_R0];
        while (*c) {
          putc((char)*c, stdout);
          ++c;
        }
        fflush(stdout);
        break;
      }

      case TRAP_IN:
        printf("Enter a char type:");
        char c = getchar();
        putc(c, stdout);
        fflush(stdout);
        reg[R_R0] = (uint16_t)c;
        update_flags(R_R0);
        break;

      case TRAP_PUTSP: {
        uint16_t *c = memory + reg[R_R0];
        while (*c) {
          char char1 = (*c) & 0xFF;
          putc(char1, stdout);
          char char2 = (*c) >> 8;
          if (char2) {
            putc(char2, stdout);
          }
          ++c;
        }
        fflush(stdout);
        break;
      }
      case TRAP_HALT:
        running = 0;
        break;
      }
      break;
    }
    }
  }

  restore_input_buffering();
  return EXIT_SUCCESS;
}

uint16_t sign_extend(uint16_t x, int bit_count) {
  // checking if most significant bit (MSB) is 1
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
}

void read_image_file(FILE *file) {
  /* the origin tells us where in memory to place the image */
  uint16_t origin;
  fread(&origin, sizeof(origin), 1, file);
  origin = swap16(origin);

  /* we know the maximum file size so we only need one fread */
  uint16_t max_read = MEMORY_MAX - origin;
  uint16_t *p = memory + origin;
  size_t read = fread(p, sizeof(uint16_t), max_read, file);

  /* swap to little endian */
  while (read-- > 0) {
    *p = swap16(*p);
    ++p;
  }
}

uint16_t read_image(char *image_path) {
  FILE *file = fopen(image_path, "rb");
  if (!file) {
    return 0;
  }
  read_image_file(file);
  fclose(file);
  return 1;
}

uint16_t swap16(uint16_t x) { return (x << 8) | (x >> 8); }

struct termios original_tio;

void disable_input_buffering() {
  tcgetattr(STDIN_FILENO, &original_tio);
  struct termios new_tio = original_tio;
  new_tio.c_lflag &= ~ICANON & ~ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}

void restore_input_buffering() {
  tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
}

uint16_t check_key() {
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(STDIN_FILENO, &readfds);

  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  return select(1, &readfds, NULL, NULL, &timeout) != 0;
}
