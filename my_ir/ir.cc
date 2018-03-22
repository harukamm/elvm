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

#include <assert.h>
#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <vector>
#include <utility>

using std::cout;
using std::endl;
using std::pair;
using std::string;
using std::vector;

typedef pair<string, vector<string> > DataPair;

class Reader {
 public:
  Reader(char* mem, int size, int pos)
    : mem(mem), size(size), pos(pos), lineno(0) { }

  ~Reader() {
    delete mem;
  }

  // Opens the given file, and returns a pointer to Reader structure. If
  // opening the file fails, returns nullptr instead.
  static Reader* create_reader_from_file(const char* filename) {
    std::ifstream fs(filename, std::ios::ate);
    if (!fs.is_open())
      return nullptr;
    int size = fs.tellg();
    char* mem = new char[size];
    fs.seekg(0, std::ios::beg);
    fs.read(mem, size);
    fs.close();
    Reader* r = new Reader(mem, size, 0);
    return r;
  }

  void dump(int begin = 0) {
    cout << "lineno:" << lineno << endl;
    for (int i = begin; i < size; i++) {
      cout << mem[i];
    }
  }

  bool is_end() const {
    return size <= pos;
  }

  int get_pos() const {
    return pos;
  }

  void set_pos(int new_pos) {
    pos = new_pos;
  }

  // Returns the next character of the given Reader. This function advances the
  // read position at the same time.
  char getc() {
    assert(!is_end());
    char c = mem[pos++];
    if (c == '\n')
      lineno++;
    return c;
  }

  // Returns the next character of the given Reader without changing the
  // current read position.
  char peek() const {
    assert(!is_end());
    return mem[pos];
  }

  // Consumes the successive spaces and returns a number of them.
  int skip_spaces() {
    int count = 0;
    while (!is_end() && isspace(peek())) {
      getc();
      count++;
    }
    return count;
  }

  // Reads consecutive identifier characters ignoring spaces before and
  // behind it.
  string token_word() {
    skip_spaces();
    string result;
    while (!is_end() && isident(peek()))
      result += getc();
    skip_spaces();
    return result;
  }

  // Skip a string literal.
  string literal() {
    assert(accept("\""));
    string result;
    bool escape = false;
    while (!is_end()) {
      char c = getc();
      if (c == '\"' && !escape)
        return result;
      escape = !escape && c == '\\';
    }
    assert(false);
  }

  // Skip consecutive non-space characters ignoring spaces before it.
  void expect(const string& expected_word) {
    skip_spaces();
    for (int i = 0; i < expected_word.size(); i++) {
      if (peek() != expected_word[i]) {
        dump(pos);
        cout << expected_word << " vs(" << i << ") " << peek() << endl;
      }
      assert(getc() == expected_word[i]);
    }
  }

  // Skip any characters until the first occurrence of '\n' or '\r'
  void skip_until_ret() {
    while(!is_end()) {
      char c = getc();
      if (c == '\n' || c == '\r')
        break;
    }
  }

  bool accept(const string& possible_word) {
    int tmp_pos = pos;
    int tmp_lineno = lineno;
    skip_spaces();
    for (int i = 0; i < possible_word.size(); i++) {
      if (is_end() || peek() != possible_word[i]) {
        //cout << possible_word << " vs( " << i << ")" << peek() << endl;
        pos = tmp_pos;
        lineno = tmp_lineno;
        return false;
      }
      getc();
    }
    return true;
  }

 private:
  const char* mem;
  int size;
  int pos;
  int lineno;
};

int string_to_int(const string& s) {
  int x = 0;
  for (int i = 0; i < s.size(); i++) {
    x *= 10;
    assert(isdigit(s[i]));
    x += s[i] - '0';
  }
  return x;
}

bool isident(const char c) {
  return isalnum(c) || c == '_' || c == '.';
}

bool is_reg(const string& s) {
  if (s.size() == 1) {
    return 'A' <= s[0] && s[0] <= 'D';
  }
  return s == "BP" || s == "SP";
}

Reg string_to_reg(const string& s) {
  assert(is_reg(s));
  if (s.size() == 1) {
    switch (s[0]) {
    case 'A':
      return A;
    case 'B':
      return B;
    case 'C':
      return C;
    case 'D':
      return D;
    default:
      assert(false);
    }
  } else {
    return s == "BP" ? BP : SP;
  }
}

Op get_op(const string& word) {
  Op o = OP_UNSET;
  if (word == "mov") {
    o = MOV;
  } else if (word == "add") {
    o = ADD;
  } else if (word == "sub") {
    o = SUB;
  } else if (word == "load") {
    o = LOAD;
  } else if (word == "store") {
    o = STORE;
  } else if (word == "eq") {
    o = EQ;
  } else if (word == "ne") {
    o = NE;
  } else if (word == "lt") {
    o = LT;
  } else if (word == "gt") {
    o = GT;
  } else if (word == "le") {
    o = LE;
  } else if (word == "ge") {
    o = GE;
  } else if (word == "jmp") {
    o = JMP;
  } else if (word == "jeq") {
    o = JEQ;
  } else if (word == "jne") {
    o = JNE;
  } else if (word == "jlt") {
    o = JLT;
  } else if (word == "jgt") {
    o = JGT;
  } else if (word == "jle") {
    o = JLE;
  } else if (word == "jge") {
    o = JGE;
  } else if (word == "putc") {
    o = PUTC;
  } else if (word == "getc") {
    o = GETC;
  } else if (word == "exit") {
    o = EXIT;
  } else if (word == "dump") {
    o = DUMP;
  }
  return o;
}

Value read_value(Reader* r) {
  assert(r != nullptr);
  r->skip_spaces();
  bool minus = r->peek() == '-';
  if (minus)
    r->getc();

  const string& word = r->token_word();
  assert(word.size() != 0);
  bool maybe_imm = isdigit(word[0]);
  assert(!minus || maybe_imm);
  Value result;
  if (maybe_imm) {
    int i = string_to_int(word);
    i *= minus ? (-1) : 1;
    result.type = IMM;
    result.imm = i;
  } else if (is_reg(word)) {
    result.type = REG;
    result.reg = string_to_reg(word);
  } else {
    result.type = LAB;
//    result.tmp = word;
  }
  return result;
}

void get_exprs(vector<Inst>* inst_list, Reader* r) {
  assert(inst_list != nullptr);
  assert(r != nullptr);
  if (r->is_end())
    return;
  while (r->accept("#") || r->accept(".loc") || r->accept(".file")
      || r->accept(".text"))
    r->skip_until_ret();

  int prev_pos = r->get_pos();
  const string& word = r->token_word();
  Op o = get_op(word);
  Inst inst;
  inst.op = o;
  Value v1, v2, v3;
  switch (o) {
  case MOV:
  case ADD:
  case SUB:
  case LOAD:
  case STORE:
  case EQ:
  case NE:
  case LT:
  case GT:
  case LE:
  case GE:
    v1 = read_value(r);
    r->expect(",");
    v2 = read_value(r);
    assert(o == MOV || v2.type != LAB);
    if (o != STORE) {
      inst.dst = v1;
      inst.src = v2;
    } else {
      inst.dst = v2;
      inst.src = v1;
    }
    break;
  case JMP:
  case JEQ:
  case JNE:
  case JLT:
  case JGT:
  case JLE:
  case JGE:
    v1 = read_value(r);
    assert(v1.type != IMM);
    inst.jmp = v1;
    if (o != JMP) {
      r->expect(",");
      v2 = read_value(r);
      assert(v2.type == REG);
      r->expect(",");
      v3 = read_value(r);
      inst.dst = v2;
      inst.src = v3;
    }
    break;
  case PUTC:
    v1 = read_value(r);
    inst.src = v1;
    assert(v1.type != LAB);
    break;
  case GETC:
    v1 = read_value(r);
    inst.src = v1;
    assert(v1.type == REG);
    break;
  case EXIT:
    break;
  case DUMP:
    break;
  case OP_UNSET:
    r->set_pos(prev_pos);
    return;
  default:
    assert(false);
  }
  inst_list->push_back(inst);
  get_exprs(inst_list, r);
}

void read_text(vector<Inst>* inst, Reader* r) {
  assert(r != nullptr);
  string label;
  while (true) {
    // Read as many expressions as possible.
    get_exprs(inst, r);
    if (r->is_end())
      break;
    int prev_pos = r->get_pos();
    const string& word = r->token_word();
    Op o = get_op(word);
    assert(o == OP_UNSET);
    if (!is_label(word) || !r->accept(":")) {
      r->set_pos(prev_pos);
      break;
    }
    label = next_label;
  }
}

vector<string> read_typevals(Reader* r) {
  assert(r != nullptr);

  vector<string> result;
  bool end = false;
  while(!r->is_end() && !end) {
    if (r->accept(".string")) {
      const string& val = r->literal();
      result.push_back(val);
    } else if (r->accept(".long")) {
      Value val = read_value(r);
      result.push_back(std::to_string(val.imm));
    } else {
      end = true;
    }
    r->skip_spaces();
  }
  return result;
}

vector<DataPair> read_data(Reader* r) {
  assert(r != nullptr);
  vector<DataPair> result;
  string label;
  while (!r->is_end()) {
    int prev_pos = r->get_pos();
    const string& next_label = r->token_word();
    if (r->accept(":")) {
      label = next_label;
    } else {
      r->set_pos(prev_pos);
    }
    const vector<string>& vals = read_typevals(r);
    if (vals.size() == 0)
      break;
    result.push_back(make_pair(label, vals));
  }
  return result;
}

Module* load_eir_impl(Reader* r) {
  assert(r != nullptr);
  vector<Inst> txt;
  vector<Data> data;

  while (!r->is_end()) {
    if (r->accept(".data")) {
      int prev_pos = r->get_pos();
      const string& number = r->token_word();
      int num = 0;
      if (number.size() != 0 && isdigit(number[0])) {
        int num = string_to_int(number);
      } else {
        r->set_pos(prev_pos);
      }
      read_data(r);
    } else {
      r->accept(".text");
      read_text(&txt, r);
    }
  }
  assert(r->is_end());
  Module* m = new Module();
  m->text = txt;
  m->data = data;
  return m;
}

string string_reg(Reg reg) {
  switch(reg) {
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
}

void dump_module(Module* m) {
  if (m == nullptr)
    return;
  dump_inst(m->text);
  dump_data(m->data);
}

// Parses the file into a Module structure which contains set of the parsed
// EIR.
Module* load_eir_from_file(const char* filename) {
  Reader* r = Reader::create_reader_from_file(filename);
  if (r == nullptr) {
    cout << "Failed to read the file" << endl;
    exit(1);
  }
  Module* m = load_eir_impl(r);
  dump_module(m);
  delete r;
  return m;
}

void test_file(const char* filename) {
  cout << " - - - test - " << filename << endl;
  load_eir_from_file(filename);
  cout << " - - - - - - - - - -" << endl;
}

int main() {/*
  test_file("../test/00exit.eir");
  test_file("../test/01putc.eir");
  test_file("../test/02mov.eir");
  test_file("../test/03mov_reg.eir");
  test_file("../test/04getc.eir");
  test_file("../test/05regjmp.eir");
  test_file("../test/06mem.eir");
  test_file("../test/07mem.eir");
  test_file("../test/08data.eir");
  test_file("../test/add_self.eir");
  test_file("../test/basic.eir");
  test_file("../test/bug_cmp.eir");
  test_file("../test/echo.eir");
  test_file("../test/isprint.eir");
  test_file("../test/neg.eir");
  test_file("../test/sub.eir");
  test_file("../test/sub_bug.eir");*/

  test_file("../out/00exit.eir");
  test_file("../out/01putc.eir");
  test_file("../out/02mov.eir");
  test_file("../out/03mov_reg.eir");
  test_file("../out/04getc.eir");
  test_file("../out/05regjmp.eir");
  test_file("../out/06mem.eir");
  test_file("../out/07mem.eir");
  test_file("../out/08data.eir");
  test_file("../out/24_cmp.c.eir");
  test_file("../out/24_cmp2.c.eir");
  test_file("../out/24_mem.eir");
  test_file("../out/24_muldiv.c.eir");
  test_file("../out/8cc.c.eir");
  test_file("../out/8cc.in.c.eir");
  test_file("../out/add_self.eir");
  test_file("../out/addsub.c.eir");
  test_file("../out/array.c.eir");
  test_file("../out/basic.eir");
  test_file("../out/bitops.c.eir");
  test_file("../out/bm_mov.eir");
  test_file("../out/bool.c.eir");
  test_file("../out/bug_cmp.eir");
  test_file("../out/cmp_eq.c.eir");
  test_file("../out/cmp_ge.c.eir");
  test_file("../out/cmp_gt.c.eir");
  test_file("../out/cmp_le.c.eir");
  test_file("../out/cmp_lt.c.eir");
  test_file("../out/cmp_ne.c.eir");
  test_file("../out/cmps.eir");
  test_file("../out/copy_struct.c.eir");
  test_file("../out/dump_ir.c.eir");
  test_file("../out/echo.eir");
  test_file("../out/elc.c.eir");
  test_file("../out/eli.c.eir");
  test_file("../out/eof.c.eir");
  test_file("../out/field_addr.c.eir");
  test_file("../out/fizzbuzz.c.eir");
  test_file("../out/fizzbuzz_fast.c.eir");
  test_file("../out/func.c.eir");
  test_file("../out/func2.c.eir");
  test_file("../out/func_ptr.c.eir");
  test_file("../out/getchar.c.eir");
  test_file("../out/global.c.eir");
  test_file("../out/global_array.c.eir");
  test_file("../out/global_struct_ref.c.eir");
  test_file("../out/hello.c.eir");
  test_file("../out/increment.c.eir");
  test_file("../out/isprint.eir");
  test_file("../out/jmps.eir");
  test_file("../out/lisp.c.eir");
  test_file("../out/logic_val.c.eir");
  test_file("../out/loop.c.eir");
  test_file("../out/malloc.c.eir");
  test_file("../out/muldiv.c.eir");
  test_file("../out/neg.eir");
  test_file("../out/nullptr.c.eir");
  test_file("../out/print_int.c.eir");
  test_file("../out/printf.c.eir");
  test_file("../out/putchar.c.eir");
  test_file("../out/puts.c.eir");
  test_file("../out/qsort.c.eir");
  test_file("../out/struct.c.eir");
  test_file("../out/sub.eir");
  test_file("../out/sub_bug.eir");
  test_file("../out/swapcase.c.eir");
  test_file("../out/switch_case.c.eir");
  test_file("../out/switch_op.c.eir");
  test_file("../out/switch_range.c.eir"); 
}
