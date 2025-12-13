# lc3
    a lightweight LC-3 (Little Computer 3) emulator written in C

## Build

```bash
gcc -Wall -Wextra -o lc3 src/main.c
./lc3 path/to/your/program.obj
```

## Features

- Full LC-3 instruction set
- `.obj` image loading
- 16-bit memory and register model
- Memory-mapped keyboard I/O
- Non-blocking input using `select`
- Terminal input control via `termios`
- Built-in trap handling (`GETC`, `OUT`, `PUTS`, `IN`, `PUTSP`, `HALT`)

## Architecture Overview

### Registers
- General-purpose registers: `R0`–`R7`
- Program Counter: `PC`
- Condition Register: `COND`

### Memory
- 16-bit address space
- Memory-mapped registers:
  - `KBSR` (Keyboard Status Register)
  - `KBDR` (Keyboard Data Register)

### Instruction Cycle
The emulator follows a standard fetch–decode–execute loop:
1. Fetch instruction from memory using `PC`
2. Decode opcode and operands
3. Execute instruction
4. Update condition flags
5. Advance `PC`

---

## Supported Instructions

- Arithmetic: `ADD`, `AND`, `NOT`
- Control flow: `BR`, `JMP`, `JSR`, `JSRR`
- Memory access: `LD`, `LDI`, `LDR`, `LEA`, `ST`, `STI`, `STR`
- System calls: `TRAP`
 
## Trap Routines

The emulator implements LC-3 trap routines directly in the VM:

| Trap | Description |
|-----|------------|
| `GETC` | Read a single character |
| `OUT` | Output a character |
| `PUTS` | Output a null-terminated string |
| `IN` | Prompt and read a character |
| `PUTSP` | Output packed characters |
| `HALT` | Stop execution |

---
## References
- [LC-3 ISA](https://www.jmeiners.com/lc3-vm/supplies/lc3-isa.pdf)
- [jmeiners LC-3 VM](https://www.jmeiners.com/lc3-vm/)

---

*I will write about it [here](https://rajatdahal.com.np).*
