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
#include <my_ir/dump_ir.h>

#include <assert.h>
#include <fstream>
#include <map>
#include <unordered_map>
#include <utility>

using std::pair;

typedef std::unordered_map<string, int> LabelRefMap;

typedef enum {
  VAL, REFLAB
} TempDataType;

typedef struct {
  TempDataType type;
  union {
    string* label;
    int val;
  };
} TempData;

typedef std::map<int, vector<TempData> > TempDataByOrd;

typedef std::map<int, LabelRefMap> LabelRefMapByOrd;

void allocate_by_ord(vector<TempData>* data, LabelRefMap* label_ref,
  const TempDataByOrd& data_by_ord, const LabelRefMapByOrd& ref_by_ord);

void dereferece_labels_text(vector<Inst>* inst,
  const LabelRefMap& txt_label_ref, const LabelRefMap& data_label_ref);

void dereferece_labels_data(vector<Data>* data,
  const vector<TempData>& tempData, const LabelRefMap& data_label_ref);

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
      if (escape) {
        escape = false;
        if (c == 'n') {
          result += '\n';
        } else if (c == 't') {
          result += '\t';
        } else if (c == '\\') {
          result += '\\';
        } else if (c == '\"') {
          result += '\"';
        } else if (c == 'x') {
          assert(!is_end());
          result += getc();
          assert(!is_end());
          result += getc();
        } else if (c == 'b') {
          result += '\b';
        } else {
          cout << c << endl;
          assert(false);
        }
      } else {
        if (c == '\"') {
          result += '\0';
          return result;
        } else if (c == '\\') {
          escape = true;
        } else {
          result += c;
        }
      }
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
    while (!is_end()) {
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
        // cout << possible_word << " vs( " << i << ")" << peek() << endl;
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
    i += MEMSZ;
    i %= MEMSZ;
    result.type = IMM;
    result.imm = i;
  } else if (is_reg(word)) {
    result.type = REG;
    result.reg = string_to_reg(word);
  } else {
    result.type = LAB;
    result.tmp = new string(word);
  }
  return result;
}

void get_exprs(vector<Inst>* inst_list, Reader* r) {
  assert(inst_list != nullptr);
  assert(r != nullptr);
  while (r->accept("#") || r->accept(".loc") || r->accept(".file")
      || r->accept(".text"))
    r->skip_until_ret();

  int prev_pos = r->get_pos();
  const string& word = r->token_word();
  Op o = OP_UNSET;
  // Get a operation code for 'word' only if it's not a declaration label.
  if (!r->accept(":"))
    o = get_op(word);
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

void read_text(vector<Inst>* inst, LabelRefMap* label_ref, Reader* r) {
  assert(inst != nullptr);
  assert(r != nullptr);
  assert(r != nullptr);
  while (true) {
    // Read as many expressions as possible.
    get_exprs(inst, r);
    if (r->is_end())
      break;
    int prev_pos = r->get_pos();
    const string& label = r->token_word();
    if (!r->accept(":")) {
      r->set_pos(prev_pos);
      break;
    }
    assert(label_ref->find(label) == label_ref->end());
    label_ref->insert(make_pair(label, inst->size()));
  }
}

int read_typevals(vector<TempData>* data, Reader* r) {
  assert(data != nullptr);
  assert(r != nullptr);
  int update = 0;
  while (!r->is_end()) {
    if (r->accept(".string")) {
      const string& str = r->literal();
      for (int i = 0; i < str.size(); i++)
        data->push_back((TempData) {VAL, .val = str[i]});
    } else if (r->accept(".long")) {
      Value val = read_value(r);
      switch (val.type) {
      case IMM:
        data->push_back((TempData) {VAL, .val = val.imm});
        break;
      case LAB:
        data->push_back((TempData) {REFLAB, val.tmp});
        break;
      default:
        assert(false);
      }
    } else {
      break;
    }
    update++;
    r->skip_spaces();
  }
  return update;
}

void read_data(vector<TempData>* data, LabelRefMap* label_ref, Reader* r) {
  assert(data != nullptr);
  assert(label_ref != nullptr);
  assert(r != nullptr);
  while (!r->is_end()) {
    int prev_pos = r->get_pos();
    const string& label = r->token_word();
    if (r->accept(":")) {
      assert(label_ref->find(label) == label_ref->end());
      label_ref->insert(make_pair(label, data->size()));
    } else {
      r->set_pos(prev_pos);
    }
    if (read_typevals(data, r) == 0)
      break;
  }
}

Module* load_eir_impl(Reader* r) {
  assert(r != nullptr);
  vector<Inst> txt;
  TempDataByOrd data_by_ord;
  LabelRefMap txt_label_ref;
  LabelRefMapByOrd data_label_ref_by_ord;

  while (!r->is_end()) {
    if (r->accept(".data")) {
      int prev_pos = r->get_pos();
      const string& number = r->token_word();
      int num = 0;
      if (number.size() != 0 && isdigit(number[0])) {
        num = string_to_int(number);
      } else {
        r->set_pos(prev_pos);
      }
      vector<TempData>* data_n = &(data_by_ord[num]);
      LabelRefMap* label_ref = &(data_label_ref_by_ord[num]);
      read_data(data_n, label_ref, r);
    } else {
      r->accept(".text");
      read_text(&txt, &txt_label_ref, r);
    }
  }

  assert(r->is_end());

  int entry = 0;
  if (txt_label_ref.find("main") != txt_label_ref.end())
    entry = txt_label_ref.find("main")->second;

  vector<TempData> tmp_data;
  LabelRefMap data_label_ref;
  allocate_by_ord(&tmp_data, &data_label_ref, data_by_ord, data_label_ref_by_ord);


  vector<Data> data;
  int dsize = static_cast<int>(tmp_data.size());
  data_label_ref.insert(std::make_pair("_edata", dsize));
  dereferece_labels_data(&data, tmp_data, data_label_ref);
  data.push_back((Data) { dsize + 1 });

  dereferece_labels_text(&txt, txt_label_ref, data_label_ref);

  Module* m = new Module();
  m->entry = entry;
  m->text = txt;
  m->data = data;
  return m;
}

void allocate_by_ord(vector<TempData>* data, LabelRefMap* label_ref,
    const TempDataByOrd& data_by_ord, const LabelRefMapByOrd& ref_by_ord) {
  assert(data != nullptr);
  assert(label_ref != nullptr);

  int all_dsize = 0;
  for (auto &p : data_by_ord)
    all_dsize += p.second.size();
  data->resize(all_dsize);

  int dsize = 0; // Current data size
  for (auto &p : data_by_ord) {
    int ord = p.first;
    const vector<TempData>& data_n = p.second;
    for (int i = 0; i < data_n.size(); i++) {
      (*data)[dsize + i] = data_n[i];
    }
    auto ref_iter = ref_by_ord.find(ord);
    if (ref_iter != ref_by_ord.end()) {
      const LabelRefMap& ref = ref_iter->second;
      for (auto &r : ref) {
        const string& name = r.first;
        int offset = r.second;
        label_ref->insert(make_pair(name, dsize + offset));
      }
    }
    dsize += data_n.size();
  }
}

void dereferece_labels_text(vector<Inst>* inst,
    const LabelRefMap& txt_label_ref, const LabelRefMap& data_label_ref) {
  assert(inst != nullptr);
  for (int i = 0; i < inst->size(); i++) {
    Inst& in = (*inst)[i];
    switch (in.op) {
    case JMP:
    case JEQ:
    case JNE:
    case JLT:
    case JGT:
    case JLE:
    case JGE:
      // Refer to a label in text.
      if (in.jmp.type == LAB) {
        string label = *in.jmp.tmp;
        auto iter = txt_label_ref.find(label);
        if (iter == txt_label_ref.end()) {
          cout << "fails!: " << label << " " << in.jmp.tmp << endl;
        }
        assert(iter != txt_label_ref.end());
        in.jmp.type = IMM;
        in.jmp.imm = iter->second;
      }
      break;
    case MOV:
      // Refer to a label in text/data.
      if (in.src.type == LAB) {
        string label = *in.src.tmp;
        auto in_dt = data_label_ref.find(label);
        auto in_txt = txt_label_ref.find(label);
        int addr;
        if (in_dt != data_label_ref.end()) {
          addr = in_dt->second;
        } else if (in_txt != txt_label_ref.end()) {
          addr = in_txt->second;
        } else {
          assert(false);
        }
        in.src.type = IMM;
        in.src.imm = addr;
      }
      break;
    default:
      break;
    }
  }
}

void dereferece_labels_data(vector<Data>* data,
    const vector<TempData>& tempData, const LabelRefMap& data_label_ref) {
  assert(data != nullptr);
  data->resize(tempData.size());
  for (int i = 0; i < tempData.size(); i++) {
    const TempData& td = tempData[i];
    int v;
    if (td.type != REFLAB) {
      v = td.val;
    } else {
      // Refer to a label in data.
      string label = *td.label;
      auto iter = data_label_ref.find(label);
      assert(iter != data_label_ref.end());
      v = iter->second;
    }
    (*data)[i].v = v;
  }
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
  // dump_module(m);
  delete r;
  return m;
}

void test_file(const char* filename) {
  // cout << " - - - test - " << filename << endl;
  Module* m = load_eir_from_file(filename);
  // cout << " - - - - - - - - - -" << endl;
  run(*m);
}

int main(int argc, char** argv) {
  if (argc == 2) {
    char* filename = argv[1];
    test_file(filename);
    return 0;
  }
}
