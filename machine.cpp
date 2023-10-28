#include "machine.h"

int LogicExpression::eval(VM &vm) {
  switch (operation) {
  case ASHL:
    return left->eval(vm) << 1;
  case ASHR:
    return left->eval(vm) >> 1;
  case AND:
    return left->eval(vm) & right->eval(vm);
  case OR:
    return left->eval(vm) | right->eval(vm);
  case XOR:
    return left->eval(vm) ^ right->eval(vm);
  case ANDNOT:
    return left->eval(vm) & ~(right->eval(vm));
  default:
    return -1; // IDK man
  };
};

class Scanner {
private:
  const std::string &stored_line;
  // std::string token;
  size_t current;
  // int line;
public:
  // Takes one line only
  // TODO: Could be made to take a full program, later
  Scanner(std::string &line) : stored_line(line), current(0){};

  bool isEndOfLine() { return current >= stored_line.length(); }

  // Return next character, then advance
  char advance() {
    char out = stored_line[current];
    current += 1;
    return out;
  }

  char peek() { return current < stored_line.length() ? stored_line[current] : '\0'; }

  // Advance index only if next character is expected
  bool match(char expected) {
    if (isEndOfLine())
      return false;
    if (stored_line[current] != expected)
      return false;
    current += 1;
    return true;
  }

  bool isDigit(char digit) { return std::isdigit(digit); };
  bool isAlpha(char alpha) { return std::isalpha(alpha); };
  bool isAlphanumeric(char c) { return isDigit(c) || isAlpha(c); };

  void skipWhitespace() {
    while (std::isspace(peek()))
      advance();
  };

  Operation ParseLongOperator() {
    skipWhitespace();
    // If first isn't an alpha, this isn't a long operator
    // so bail immediately
    if (!isAlpha(peek()))
      return Operation::Invalid;
    // Save start of token. Crafting Interpreters has this as a class global
    // probably more efficient to reset it per token scan instead
    int start = current;
    // Advance this.current until we hit a non-alphanumeric, like a space or an
    // operator This is so we don't eat the space
    while (isAlphanumeric(peek()))
      advance();
    // Compare the substring to a list of identifiers
    if (stored_line.compare(start, current, "ashl")) {
      return Operation::ASHL;
    } else if (stored_line.compare(start, current, "ashr")) {
      return Operation::ASHR;
    } else {
      return Operation::Invalid;
    }
  }

  Operation ParseOperator() {
    skipWhitespace();
    if (match('&')) {
      return Operation::AND;
    } else if (match('^')) {
      return Operation::XOR;
    } else if (match('<')) {
      return match('-') ? Operation::Move : Operation::Invalid;
    } else {
      return Operation::Invalid;
      // return ParseLongOperator();
    }
  };

  Address *ParseAddress() {
    skipWhitespace();
    Address *address = new Address();
    char c = advance();
    if (c >= 'A' && c <= 'G') {
      address->index = c - 'A';
      address->type = MemoryType::Register;
    } else if (c == 'P') {
      address->index = -1;
      address->type = MemoryType::Output;
    } else {
      address->index = -1;
      address->type = MemoryType::Null;
    }
    return address;
  };

  BinaryExpression *ParseBinaryExpression() {
    skipWhitespace();
    int start = current;
    while (peek() == '0' || peek() == '1')
      advance();

    BinaryExpression *binary = new BinaryExpression();
    try {
      binary->number =
          std::stoi(stored_line.substr(start, current), nullptr, 2);
    } catch (std::exception &e) {
      // std::cout << "Failed to parse number: "
      //           << stored_line.substr(start, current) << std::endl;
      //  Reset counter
      this->current = start;
      binary->err = true;
      return binary;
    }
    //std::cout << "Parsed " << binary->number << std::endl;
    return binary;
  }

  LogicExpression *ParseLogicExpression() {
    skipWhitespace();
    LogicExpression *logic = new LogicExpression();
    int start = current;

    // Test for unary operators
    Operation op = ParseLongOperator();
    Address *operand = ParseAddress();
    // this is to test for end of line, can't allow trailing tokens
    skipWhitespace();
    if (op != Operation::Invalid && operand->index != -1 && isEndOfLine()) {
      // Success
      logic->left = operand;
      logic->operation = op;
      return logic;
    }

    // Else reset and try for a binary operation
    this->current = start;
    Address *left = ParseAddress();
    op = ParseOperator();
    Address *right = ParseAddress();
    skipWhitespace();
    if (left->index != -1 && right->index != -1 && op != Operation::Invalid &&
        isEndOfLine()) {
      logic->left = left;
      logic->operation = op;
      logic->right = right;
      return logic;
    };

    // Else error out
    logic->err = true;
    return logic;
  };

  Expression *ParseExpression() {
    int start = current;
    // Try parsing a binary
    BinaryExpression *binary = ParseBinaryExpression();
    if (!binary->err)
      return binary;
    current = start;

    LogicExpression *logic = ParseLogicExpression();
    if (!logic->err)
      return logic;
    current = start;

    Address *address = ParseAddress();
    skipWhitespace();
    if(!address->err && isEndOfLine())
      return address;
    // Return error if we got here
    Expression *err = new Expression();
    err->err = true;
    return err;
  }
};

Command ParseCommand(std::string &line) {
  Command comm;
  comm.err = false;
  comm.noop = false;
  Scanner scanner(line);

  scanner.skipWhitespace();
  if(scanner.isEndOfLine() || scanner.match(';')) {
    comm.noop = true;
    return comm;
  }

  // Get target address
  comm.target = scanner.ParseAddress();
  if (comm.target->type == MemoryType::Null) {
    comm.err = true;
    return comm;
  }

  // Get arrow
  if (scanner.ParseOperator() != Operation::Move) {
    comm.err = true;
    std::cout << "Failed to find arrow" << std::endl;
    return comm;
  }

  // Get left expression
  comm.expression = scanner.ParseExpression();
  if (comm.expression->err) {
    comm.err = true;
    return comm;
  }

  return comm;
}

int main(int argc, char *argv[]) {
  if (argc != 2 && argc != 3) {
    std::cout << "source [no-output | yes-output]"
              << std::endl;
    return 1;
  };

  bool dumpRegisters = true;
  if(argc == 3 && std::string("no-output").compare(argv[2]) == 0) {
    dumpRegisters = false;
  }

  std::ifstream programFile;
  programFile.open(argv[1], std::ios::in);
  if (!programFile.is_open()) {
    std::cout << "Cannot open file" << std::endl;
    return 1;
  }

  std::vector<std::string> programLines;
  while (!programFile.eof()) {
    std::string line;
    std::getline(programFile, line);
    programLines.push_back(line);
  }

  VM vm;
  for (std::string &line : programLines) {
    Command comm = ParseCommand(line);
    comm.execute(vm);
    if (comm.err)
      std::cout << "Error: " << line << std::endl;
  }

  if(dumpRegisters) vm.printState();

  return 0;
}
