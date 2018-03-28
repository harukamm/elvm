// Copyright 2018 Haruka Matsumoto

#include <my_ir/dump_ir.h>

string string_reg(Reg reg) {
  switch (reg) {
  case A:
    return "A";
  case B:
    return "B";
  case C:
    return "C";
  case D:
    return "D";
  case SP:
    return "SP";
  case BP:
    return "BP";
  default:
    return "none";
  }
}

string string_op(Op o) {
  switch (o) {
  case MOV:
    return "MOV";
  case ADD:
    return "ADD";
  case SUB:
    return "SUB";
  case LOAD:
    return "LOAD";
  case STORE:
    return "STORE";
  case PUTC:
    return "PUTC";
  case GETC:
    return "GETC";
  case EXIT:
    return "EXIT";
  case JEQ:
    return "JEQ";
  case JNE:
    return "JNE";
  case JLT:
    return "JLT";
  case JGT:
    return "JGT";
  case JLE:
    return "TLE";
  case JGE:
    return "JGE";
  case JMP:
    return "JMP";
  case EQ:
    return "EQ";
  case NE:
    return "NE";
  case LT:
    return "LT";
  case GT:
    return "GT";
  case LE:
    return "LE";
  case GE:
    return "GE";
  case DUMP:
    return "DUMP";
  default:
    return "NONE";
  }
}

void dump_value(const Value& val) {
  switch (val.type) {
  case REG:
    cout << "reg/" << string_reg(val.reg);
    break;
  case IMM:
    cout << "imm/" << val.imm;
    break;
  case LAB:
    cout << "tmp/" << val.tmp;
    break;
  default:
    cout << "none";
  }
}

void dump_inst(const vector<Inst>& inst) {
  for (int i = 0; i < inst.size(); i++) {
    cout << i << "-th,";
    cout << string_op(inst[i].op);
    cout << " dst: ";
    dump_value(inst[i].dst);
    cout << " src: ";
    dump_value(inst[i].src);
    cout << " jmp: ";
    dump_value(inst[i].jmp);
    cout << endl;
  }
}

void dump_data(const vector<Data>& data) {
  for (int i = 0; i < data.size(); i++) {
    cout << "[" << i << "] " << data[i].v << endl;
  }
}

void dump_module(Module* m) {
  if (m == nullptr)
    return;
  dump_inst(m->text);
  dump_data(m->data);
}

