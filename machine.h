#ifndef MACHINE_H_
#define MACHINE_H_

#include <cstdint>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

struct VM {
  int8_t registers[8] = {0};
  void printState() {
    std::cout << "Registers" << std::endl;
    for (int i = 0; i < 8; i++) {
      std::cout << "R" << i << " = " << (int)registers[i] << " = "<<std::bitset<8>(registers[i]) << std::endl;
    }
  }
};

enum MemoryType { Null, Register, Output };

enum Operation {
  Invalid,
  Arithmetic,
  Move,
  Negate,
  AND,
  OR,
  XOR,
  ANDNOT,
  ASHL,
  ASHR
};

struct Expression {
  bool err;
  // NOTE this will bite later
  virtual int eval(VM &) { return 101001; };
};

struct Address : public Expression {
  MemoryType type = MemoryType::Null;
  int index = 0;
  virtual int eval(VM &vm) override { return vm.registers[index]; }
};

// FIXME make this extend expression and throw this everywhere instead?
struct Error {
  int linenumber = -1;
};

struct BinaryExpression : public Expression {
  int number;
  virtual int eval(VM &) override {
    return number;
  };
};

struct LogicExpression : public Expression {
  Operation operation;
  Address* left;
  Address* right;

  virtual int eval(VM &vm) override;
};

/*
struct ArithmeticExpression : public Expression {
        Operation operation;
        Address left, right;
        bool carry;
}*/

// Each command is essentially an expression
// transfering a number to a register
struct Command {
  bool err = false;
  bool noop = false; // Sometimes
  Address* target;
  Expression* expression;

  void execute(VM &vm) {
    if (!err && !noop && target && expression) {
      if(target->type == MemoryType::Register)
        vm.registers[target->index] = expression->eval(vm);
      else if(target->type == MemoryType::Output)
        OutputValue(expression->eval(vm));
    }
  }

  void OutputValue(int value) {
    std::cout << value << " = " << std::bitset<8>(value) << std::endl;
  };
};

#endif // MACHINE_H_
