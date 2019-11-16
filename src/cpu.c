#include "cpu.h"

#include "gfx.h"

#define MEM_SIZE 4096
#define STACK_SIZE 16

struct cpu_t {
  // MEMORY
  uint8_t mem[MEM_SIZE];

  // REGISTERS
  uint8_t v[16];

  // FLAG REGISTER
  uint8_t vf;

  // INDEX REGISTER
  uint16_t i;

  // STACK
  uint16_t stack[STACK_SIZE];
  uint16_t sp;

  // PROGRAM COUNTER
  uint16_t pc;
} cpu;

#define FONTSET_SIZE 80
uint8_t fontset[FONTSET_SIZE] = {
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void cpu_reset() {

  for (uint32_t i = 0; i < MEM_SIZE; i++) {
    cpu.mem[i] = 0;
  }
  for (uint32_t i = 0; i < 16; i++) {
    cpu.v[i] = 0;
    cpu.stack[i] = 0;
  }
  cpu.i = 0; 
  cpu.pc = 0;
  cpu.sp = 0;

  // load fontset into memory
  for (uint32_t i = 0; i < FONTSET_SIZE; i++) {
    cpu.mem[i] = fontset[i];
  }
}

void cpu_load(char *path) {

  FILE *file = fopen(path, "rb");
  if (!file) {
    error("unable to open file %s\n", path);
  }

  fread(cpu.mem + 0x200, MEM_SIZE - 0x200, 1, file);

  fclose(file);
}

inline uint16_t push(uint16_t add) {
  if (cpu.sp >= STACK_SIZE - 1) {
    error("stack size exceeded\n");
  }
  return cpu.stack[cpu.sp++] = add;
}

inline uint16_t pop() {
  if (cpu.sp <= 1) {
    error("unable to pop stack\n");
  }
  return cpu.stack[cpu.sp--];
}

int cpu_spin() {

#define GETO(n) ((op >> (8 * n)) & 0xf)

  int draw_flag = 0;

  // FETCH
  uint16_t op = cpu.mem[cpu.pc] << 8 | cpu.mem[cpu.pc + 1];
  pc += 2;
  
  // DECODE 
  // EXECUTE
  switch (GETO(3)) {
    case 0:
      if (op == 0x00e0) {
        // clear screen
        screen_clear();
      } else if (op == 0x00ee) {
        // return
        cpu.pc = pop();
      } else {
        // call 
        cpu.pc = push(op & 0x0fff);        
      }
      break;
    case 1: 
      // jump
      cpu.pc = op & 0x0fff;
      break;
    case 2: 
      // call
      cpu.pc = push(op & 0x0fff);
      break;
    case 3: 
      if (cpu.v[GETO(2)] == (op & 0x00ff)) {
        cpu.pc += 2;
      }
      break;
    case 4: 
      if (cpu.v[GETO(2)] != (op & 0x00ff)) {
        cpu.pc += 2;
      }
      break;
    case 5:
      if (cpu.v[GETO(2)] == cpu.v[GETO(1)]) {
        cpu.pc += 2;
      }
      break;
    case 6:
      cpu.v[GETO(2)] = op & 0x00ff;
      break;
    case 7:
      cpu.v[GETO(2)] += op & 0x00ff;
      break;
    case 8:
      switch (GETO(0)) {
        case 0:
          cpu.v[GETO(2)] = cpu.v[GETO(1)];
          break;
        case 1:
          cpu.v[GETO(2)] |= cpu.v[GETO(1)];
          break;
        case 2:
          cpu.v[GETO(2)] &= cpu.v[GETO(1)];
          break;
        case 3:
          cpu.v[GETO(2)] ^= cpu.v[GETO(1)];
          break;
        case 4:
          {
            uint16_t r = cpu.v[GETO(2)] + cpu.v[GETO(1)];
            cpu.vf = (r >= 255);
            cpu.v[GETO(2)] = (uint8_t) r;
            break;
          }
        case 5:
          {
            uint16_t r = cpu.v[GETO(2)] - cpu.v[GETO(1)];
            cpu.vf = (r >= 255);
            cpu.v[GETO(2)] = (uint8_t) r;
            break;
          }
        case 6:
          cpu.vf = cpu.v[GETO(2)] & 1;
          cpu.v[GETO(2)] >>= 1;
          break;
        case 7:
          {
            uint16_t r = cpu.v[GETO(1)] - cpu.v[GETO(2)];
            cpu.vf = (r >= 255);
            cpu.v[GETO(2)] = (uint8_t) r;
            break;
          }
        case 8:
          cpu.vf = cpu.v[GETO(2)] & 1;
          cpu.v[GETO(2)] >>= 1;
          break;
      }
    }

  }

  return draw_flag;
}


