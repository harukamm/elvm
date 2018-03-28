// Copyright 2018 Haruka Matsumoto

#include <my_ir/ir.h>
#include <my_ir/dump_ir.h>

#include <assert.h>

#define MEMSZ 0x1000000

using std::cin;

int mem[MEMSZ];
int regs[6];

int value(const Value& v) {
  if (v.type == REG) {
    return regs[v.reg];
  } else if (v.type == IMM) {
    return v.imm;
  } else {
    assert(false);
  }
}

int src(const Inst& inst) {
  return value(inst.src);
}

int jmp(const Inst& inst) {
  return value(inst.jmp);
}

int cmp(const Inst& inst) {
  if (inst.op == JMP)
    return 1;
  assert(inst.dst.type == REG);
  int v1 = value(inst.dst);
  int v2 = value(inst.src);
  switch (inst.op) {
  case EQ:
  case JEQ:
    return v1 == v2;
  case NE:
  case JNE:
    return v1 != v2;
  case LT:
  case JLT:
    return v1 < v2;
  case GT:
  case JGT:
    return v2 < v1;
  case LE:
  case JLE:
    return v1 <= v2;
  case GE:
  case JGE:
    return v2 <= v1;
  default:
    assert(false);
  }
}

void run(const Module& m) {
  const vector<Inst> text = m.text;
  const vector<Data> data = m.data;

  int pc = m.entry;
  int npc = -1;
  int addr;
  char c;

  for (int i = 0; i < data.size(); i++) {
    mem[i] = data[i].v;
  }

  for (;; pc = npc != -1 ? npc : (pc + 1), npc = -1) {
    if (pc < 0 || text.size() <= pc)
      assert(false);

    Inst inst = text[pc];
    cout << string_op(inst.op) << endl;
    switch (inst.op) {
    case MOV:
      assert(inst.dst.type == REG);
      regs[inst.dst.reg] = src(inst);
      break;
    case ADD:
      assert(inst.dst.type == REG);
      regs[inst.dst.reg] += src(inst);
      regs[inst.dst.reg] += MEMSZ;
      regs[inst.dst.reg] %= MEMSZ;
      break;
    case SUB:
      assert(inst.dst.type == REG);
      regs[inst.dst.reg] -= src(inst);
      regs[inst.dst.reg] += MEMSZ;
      regs[inst.dst.reg] %= MEMSZ;
      break;
    case LOAD:
      assert(inst.dst.type == REG);
      addr = src(inst);
      assert(0 <= addr && addr < MEMSZ);
      regs[inst.dst.reg] = mem[addr];
      break;
    case STORE:
      assert(inst.src.type == REG);
      addr = value(inst.dst);
      assert(0 <= addr && addr < MEMSZ);
      mem[addr] = regs[inst.src.reg];
      break;
    case EQ:
    case NE:
    case LT:
    case GT:
    case LE:
    case GE:
      regs[inst.dst.reg] = cmp(inst);
      break;
    case JMP:
    case JEQ:
    case JNE:
    case JLT:
    case JGT:
    case JLE:
    case JGE:
      if (cmp(inst)) {
        npc = jmp(inst);
        cout << "jump to: " << npc << endl;
      }
      break;
    case PUTC:
      c = value(inst.src);
      cout << "put: " << c << endl;
      break;
    case GETC:
      assert(inst.src.type == REG);
      cin >> c;
      regs[inst.src.reg] = c;
      break;
    case EXIT:
      return;
      //exit(0);
    case DUMP:
      break;
    default:
      assert(false);
    }
  }
}

