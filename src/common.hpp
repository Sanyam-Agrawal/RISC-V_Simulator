#ifndef __COMMON_H
#define __COMMON_H

#include <iomanip>  // for formatting traces
#include <iostream> // for dumping trace to output stream
#include <utility>  // for std::pair

using Cycle = std::size_t;

// width of an integer register in bits, practically the word size
constexpr int XLEN = 32;
using Word = std::uint32_t;
using SignedWord = std::int32_t;

// maximum instruction length supported
constexpr int ILEN = 32;
using Instruction = std::uint32_t;

// 32 registers: r0, r1, ..., r31
constexpr unsigned no_of_registers = 32;

#endif /* end of __COMMON_H */
