#ifndef ELVM_IR_H_
#define ELVM_IR_H_

#include <stdio.h>

#define UINT_MAX 16777215
#define UINT_MAX_STR "16777215"
#ifdef __eir__
# define MOD24(v) v
#else
# define MOD24(v) v & UINT_MAX
#endif

#include <string>
#include <vector>

using std::string;
using std::vector;

typedef enum {
  A, B, C, D, BP, SP
} Reg;

typedef enum {
  REG, IMM, LAB
} ValueType;

typedef enum {
  OP_UNSET = -2, OP_ERR = -1,
  MOV = 0, ADD, SUB, LOAD, STORE, PUTC, GETC, EXIT,
  JEQ = 8, JNE, JLT, JGT, JLE, JGE, JMP,
  // Optional operations follow.
  EQ = 16, NE, LT, GT, LE, GE, DUMP,
  LAST_OP
} Op;

typedef struct {
  ValueType type;
  union {
    Reg reg;
    int imm;
    string* tmp;
  };
} Value;

typedef struct {
  Op op;
  Value dst;
  Value src;
  Value jmp;
  int pc;
  int lineno;
} Inst;

typedef struct {
  int v;
} Data;

typedef struct {
  vector<Inst> text;
  vector<Data> data;
} Module;

Module* load_eir(FILE* fp);

Module* load_eir_from_file(const char* filename);

void split_basic_block_by_mem();

bool isident(const char c);

#ifdef __GNUC__
#if __has_attribute(fallthrough)
#define FALLTHROUGH __attribute__((fallthrough))
#else
#define FALLTHROUGH
#endif
#else
#define FALLTHROUGH
#endif

#endif  // ELVM_IR_H_
