#include <cstdint>
#define MEMORY_MAX (1 << 16)

inline uint16_t memory[MEMORY_MAX]; // array of possible addresses

enum {
  R_R0 = 0,
  R_R1,
  R_R2,
  R_R3,
  R_R4,
  R_R5,
  R_R6,
  R_R7,
  R_PC, // program counter
  R_COND,
  R_COUNT
};

inline uint16_t reg[R_COUNT]; // array of registers

enum {
  OP_BR = 0, /* Branch */
  OP_ADD,    /* Add  */
  OP_LD,     /* Load */
  OP_ST,     /* Store */
  OP_JSR,    /* Jump register */
  OP_AND,    /* Bitwise and */
  OP_LDR,    /* Load register */
  OP_STR,    /* Store register */
  OP_RTI,    /* Unused */
  OP_NOT,    /* Bitwise not */
  OP_LDI,    /* Load indirect */
  OP_STI,    /* Store indirect */
  OP_JMP,    /* Jump */
  OP_RES,    /* Reserved (unused) */
  OP_LEA,    /* Load effective address */
  OP_TRAP    /* Execute trap */
};

enum {
  FL_POS = 1 << 0,
  FL_ZRO = 1 << 1,
  FL_NEG = 1 << 2,
};

