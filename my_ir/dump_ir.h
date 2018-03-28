// Copyright 2018 Haruka Matsumoto

#include <my_ir/ir.h>

#include <iostream>

using std::cout;
using std::endl;

string string_reg(Reg reg);

string string_op(Op o);

void dump_value(const Value& val);

void dump_inst(const vector<Inst>& inst);

void dump_data(const vector<Data>& data);

void dump_module(Module* m);

