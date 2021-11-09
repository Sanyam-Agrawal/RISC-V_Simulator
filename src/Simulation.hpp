#ifndef __SIMULATION_H
#define __SIMULATION_H

#include "Memory.hpp"
#include "RegisterFile.hpp"
#include <string>

class Simulation final {

  Memory memory;
  RegisterFile RF;

  const std::string binary_path;

  Word initialize();

  std::pair<Word, Cycle> execute(const Instruction, Word);

public:
  Simulation(const Memory &memory_, const std::string binary_path_)
      : memory(memory_), RF(), binary_path(binary_path_) {
    static_assert(XLEN == ILEN,
                  "This simulator only works for RISCV RV32I base ISA.");
  }

  void simulate();
};

#endif /* end of __SIMULATION_H */
