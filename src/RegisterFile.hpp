#ifndef __REGISTER_FILE_H
#define __REGISTER_FILE_H

#include "common.hpp"

class RegisterFile final {

  std::array<Word, no_of_registers> RF;

public:
  RegisterFile() : RF{} {
    // Register r0 is hardwired with all bits equal to 0.
    RF[0] = 0;
  }

  Word getReg(const unsigned idx) {
    if (idx >= no_of_registers)
      throw std::runtime_error("invalid register name");
    return RF[idx];
  }

  void writeReg(const unsigned idx, const Word val) {
    if (idx >= no_of_registers)
      throw std::runtime_error("invalid register name");
    if (idx != 0) // r0 is read-only, so all writes are discarded
      RF[idx] = val;
  }

  void dump(std::ostream &os) {
    // formatting change as right looks bad
    os << std::left;

    for (unsigned i = 0; i < no_of_registers; ++i) {
      os << (i < 10 ? " " : "") << "r" << i << " : ";
      os << std::hex << std::setw(XLEN / 4) << RF[i] << std::dec;
      os << ((i & 0b11) == 0b11 ? '\n' : ' ');
    }

    // reset formatting change
    os << std::right;
  }
};

#endif /* end of __REGISTER_FILE_H */
