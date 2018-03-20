// Copyright 2018 Haruka Matsumoto
//
// <simm> := <signed int>
// <reg> := A
//        | B
//        | C
//        | D
//        | BP
//        | SP
// <exp> := mov   <reg>, <reg|simm|label>
//        | add   <reg>, <reg|simm>
//        | sub   <reg>, <reg|simm>
//        | load  <reg>, <reg|simm>
//        | store <reg>, <reg|simm>
//        | eq    <reg>, <reg|simm>
//        | ne    <reg>, <reg|simm>
//        | lt    <reg>, <reg|simm>
//        | gt    <reg>, <reg|simm>
//        | le    <reg>, <reg|simm>
//        | ge    <reg>, <reg|simm>
//        | jmp   <reg|label>
//        | jeq   <reg|label>, <reg>, <reg|simm>
//        | jne   <reg|label>, <reg>, <reg|simm>
//        | jlt   <reg|label>, <reg>, <reg|simm>
//        | jgt   <reg|label>, <reg>, <reg|simm>
//        | jle   <reg|label>, <reg>, <reg|simm>
//        | jge   <reg|label>, <reg>, <reg|simm>
//        | putc  <reg|simm>
//        | getc  <reg>
//        | exit
// <text> := [<exp>] <text>
//         | [ <label> : [<exp>] ]
// <type_and_value> := .string <string>
//                   | .long <int>
// <data> := [ <label> : <type_and_val> ]
// <segs> := .text <text> .data <data>
//         | .data <data> .text <text>

#include <my_ir/ir.h>

#include <iostream>
#include <list>

using std;

typedef struct {
  char* mem;
  int size;
  int pos;

  bool is_end() const {
    return size <= pos;
  }
} Reader;

// Returns the next character of the given Reader. This function advances the
// read position at the same time.
char getc(Reader* r) {
  assert(r != nullptr);
  assert(!r->is_end());
  return r->mem[pos++];
}

// Returns the next character of the given Reader without changing the current
// read position.
char peek(Reader* r) {
  assert(r != nullptr);
  assert(!r->is_end());
  return r->mem[pos];
}

// Opens the given file, and returns a pointer to Reader structure. If opening
// the file fails, returns nullptr instead.
Reader* read_file(const char* filename) {
  ofstream file(filename);
  if (!file.is_open())
    return nullptr;
  int size = file.tellg();
  char* mem = new char[size];
  file.seekg(0, ios::beg);
  file.read(mem, size);
  file.close();
  Reader* r = new Reader(mem, size, 0);
  return r;
}

Module* load_eir_impl(const char* filename, Reader* r) {
  return nullptr;
}

// Parses the file into a Module structure which contains set of the parsed
// EIR.
Module* load_eir_from_file(const char* filename) {
  Reader* r = read_file(filename);
  if (r == nullptr) {
    cout << "Failed to read the file";
    exit(1);
  }
  Module* m = load_eir_impl(filename, r);
  delete r;
  return m;
}

