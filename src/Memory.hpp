#ifndef __MEMORY_H
#define __MEMORY_H

#include "common.hpp"
#include <vector> // for std::vector

class Memory final {

  const Word size;
  const Cycle access_time;

  // used for warning on writes to program memory
  Word program_begin, program_end;

  // non word-aligned memory accesses are illegal according to documentation
  // so, for ease of implementation, we use a flat memory with word-sized elements
  std::vector<Word> mem;

public:
  Memory(const Cycle access_time_, const Word size_ = 256)
      : size(size_), access_time(access_time_), mem(size_) {}

  void set_program_memory(Word begin, Word end) {
    program_begin = begin;
    program_end = end;
  }

  std::pair<Word, Cycle> getData(Word idx) {
    if (idx & 3)
      throw std::runtime_error("unaligned memory access");
    idx /= 4;
    if (idx >= size)
      throw std::runtime_error("index outside memory bounds");
    return {mem[idx], access_time};
  }

  Cycle writeData(Word idx, const Word val) {
    if (idx & 3)
      throw std::runtime_error("unaligned memory access");
    if (program_begin <= idx and idx < program_end)
      std::cerr << "WARNING: write to program memory, may make program ill-formed\n";
    idx /= 4;
    if (idx >= size)
      throw std::runtime_error("index outside memory bounds");
    mem[idx] = val;
    return access_time;
  }

  void dump(std::ostream &os) {
    os << "Main Memory\n";
    os << "===========\n";

    // formatting changes
    char prev_fill = os.fill('0');
    os << std::hex;

    for (Word i = 0; i < size; ++i) {
      if (!(i & 0b11))
        os << "0x" << std::setw(XLEN / 4) << i * 4 << " : ";
      os << "0x" << std::setw(XLEN / 4) << mem[i];
      os << ((i & 0b11) == 0b11 ? '\n' : ' ');
    }

    // reset formatting changes
    os << std::dec;
    os.fill(prev_fill);
  }
};

#endif /* end of __MEMORY_H */
