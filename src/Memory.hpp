#ifndef __MEMORY_H
#define __MEMORY_H

#include "common.hpp"
#include <list>     // for std::list
#include <optional> // for std::optional
#include <random>   // for random number
#include <vector>   // for std::vector

class MainMemory final {

  const Word size;
  const Cycle access_time;

  // non word-aligned memory accesses are illegal according to documentation
  // so, for ease of implementation, we use a flat memory with word-sized elements
  std::vector<Word> mem;

  friend class Cache;

  std::pair<std::vector<Word>, Cycle> getBlock(Word idx, Word num) {
    idx /= 4;
    if (idx + num > size)
      throw std::runtime_error("block outside memory bounds");
    std::vector<Word> block(mem.begin() + idx, mem.begin() + idx + num);
    return {block, access_time};
  }

  Cycle writeBlock(Word idx, const std::vector<Word> &block) {
    idx /= 4;
    if (idx + block.size() > size)
      throw std::runtime_error("block outside memory bounds");
    std::copy(block.begin(), block.end(), mem.begin() + idx);
    return access_time;
  }

public:
  MainMemory(const Cycle access_time_ = 100, const Word size_ = 256)
      : size(size_), access_time(access_time_), mem(size_) {}

  std::pair<Word, Cycle> getData(Word idx) {
    idx /= 4;
    if (idx >= size)
      throw std::runtime_error("index outside memory bounds");
    return {mem[idx], access_time};
  }

  Cycle writeData(Word idx, const Word val) {
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

enum class WritePolicy { WriteBack, WriteThrough };

enum class ReplacementPolicy { LRU, RANDOM, FIFO };

class Cache final {
  MainMemory *memory = nullptr;
  Word hits = 0, misses = 0;

  const Word size, block_size, associativity;
  const Cycle miss_penalty, hit_time;
  const WritePolicy WP;
  const ReplacementPolicy RP;

  const Word offset_bits, index_bits, tag_bits;

  struct CacheTableEntry {
    Word index = 0, tag = 0;
    std::vector<Word> data;
    bool isActive = false, isDirty = false;
    CacheTableEntry(Word size_) : data(size_) {}
  };

  std::vector<CacheTableEntry> table;

  std::vector<std::list<CacheTableEntry *>> setOrder;

  static Word takeLog(const Word x) {
    for (Word i = 0; i < XLEN; ++i)
      if ((1u << i) == x)
        return i;
    throw std::runtime_error("not a power of 2");
  }

  Word getOffset(Word address) { return address & ((1u << offset_bits) - 1); }

  Word getIndex(Word address) { return (address >> offset_bits) & ((1u << index_bits) - 1); }

  Word getTag(Word address) { return address >> (index_bits + offset_bits); }

  Word getAddress(const Word tag, const Word index, const Word offset) {
    Word address = 0;
    address |= tag;
    address <<= index_bits;
    address |= index;
    address <<= offset_bits;
    address |= offset;
    return address;
  }

  CacheTableEntry *getReplacementBlock(const Word index) {
    static std::mt19937_64 rng(std::random_device{}());
    switch (RP) {
    case ReplacementPolicy::RANDOM: {
      Word choice = std::uniform_int_distribution<int>(0, associativity - 1)(rng);
      return &table[index * associativity + choice];
    }
    case ReplacementPolicy::LRU:
    case ReplacementPolicy::FIFO: {
      auto victim = setOrder[index].front();
      setOrder[index].pop_front();
      setOrder[index].push_back(victim);
      return victim;
    }
    default:
      throw std::runtime_error("wtaf");
    }
  }

  std::pair<CacheTableEntry *, Cycle> getTableEntry(const Word address) {
    const Word index = getIndex(address), tag = getTag(address);
    for (Word idx = 0; idx < associativity; ++idx) {
      auto tableEntry = &table[index * associativity + idx];
      if (tableEntry->isActive and tableEntry->tag == tag) {
        if (tableEntry->index != index)
          throw std::runtime_error("cache in inconsistent state");
        ++hits;
        if (RP == ReplacementPolicy::LRU) {
          setOrder[index].remove(tableEntry);
          setOrder[index].push_back(tableEntry);
        }
        return {tableEntry, hit_time};
      }
    }
    ++misses;
    auto [block, t_mem] = memory->getBlock(getAddress(tag, index, 0), block_size);
    auto tableEntry = getReplacementBlock(index);
    // if victim is dirty, write it to memory
    if (tableEntry->isDirty)
      t_mem += memory->writeBlock(getAddress(tableEntry->tag, tableEntry->index, 0), tableEntry->data);
    // replace victim with new entry
    tableEntry->index = index;
    tableEntry->tag = tag;
    std::copy(block.begin(), block.end(), tableEntry->data.begin());
    tableEntry->isActive = true;
    tableEntry->isDirty = false;
    return {tableEntry, hit_time + miss_penalty + t_mem};
  }

public:
  // size, block_size are in terms of Word
  Cache(Word size_ = 32, Cycle miss_penalty_ = 4, Cycle hit_time_ = 10,
        Word block_size_ = 2, Word associativity_ = 2,
        WritePolicy WP_ = WritePolicy::WriteThrough,
        ReplacementPolicy RP_ = ReplacementPolicy::LRU)
      : size(size_), block_size(block_size_), associativity(associativity_),
        miss_penalty(miss_penalty_), hit_time(hit_time_), WP(WP_), RP(RP_),
        offset_bits(takeLog(block_size * (XLEN / 8))),
        index_bits(takeLog(size / block_size / associativity)),
        tag_bits(XLEN - offset_bits - index_bits),
        table(size / block_size, {block_size}),
        setOrder(size / block_size / associativity) {
    Word no_of_sets = 1u << index_bits;
    for (Word i = 0; i < no_of_sets; ++i)
      for (Word j = 0; j < associativity; ++j)
        setOrder[i].push_back(&table[i * associativity + j]);
  }

  Cache(Cache &) = delete;
  Cache(Cache &&) = delete;

  void setMemory(MainMemory *memory_) { memory = memory_; }

  std::pair<Word, Cycle> getData(const Word idx) {
    auto [tableEntry, t] = getTableEntry(idx);
    Word i = getOffset(idx) / 4;
    return {tableEntry->data[i], t};
  }

  Cycle writeData(const Word idx, const Word val) {
    auto [tableEntry, t] = getTableEntry(idx);
    Word i = getOffset(idx) / 4;
    tableEntry->data[i] = val;
    if (WP == WritePolicy::WriteThrough)
      t += memory->writeData(idx, val);
    else
      tableEntry->isDirty = true;
    return t;
  }

  void dump(std::ostream &os) {
    os << "Cache\n";
    os << "=====\n";

    os << "Hits: " << hits << "\tMisses: " << misses << "\n";
    os << "Miss Rate: " << 100 * static_cast<long double>(misses) / (hits + misses) << "%\n";

    // formatting changes
    char prev_fill = os.fill('0');
    os << std::hex;

    for (CacheTableEntry &tableEntry : table) {
      if (not tableEntry.isActive)
        continue;
      os << "0x" << std::setw(XLEN / 4)
         << getAddress(tableEntry.tag, tableEntry.index, 0) << " : ";
      for (auto i : tableEntry.data)
        os << "0x" << std::setw(XLEN / 4) << i << " ";
      os << "\n";
    }

    // reset formatting changes
    os << std::dec;
    os.fill(prev_fill);
  }
};

class Memory final {

  MainMemory *mainMemory;
  std::optional<Cache *> cache;

  // used for warning on writes to program memory
  Word program_begin, program_end;

public:
  Memory(MainMemory *mainMemory_) : mainMemory(mainMemory_) {}

  Memory(MainMemory *mainMemory_, Cache *cache_)
      : mainMemory(mainMemory_), cache(cache_) {
    if (cache)
      cache.value()->setMemory(mainMemory);
  }

  Memory(const Memory &other)
      : mainMemory(other.mainMemory), cache(other.cache),
        program_begin(other.program_begin), program_end(other.program_end) {
    if (cache)
      cache.value()->setMemory(mainMemory);
  }

  Memory(Memory &&) = delete;

  void set_program_memory(const Word begin, const Word end) {
    program_begin = begin;
    program_end = end;
  }

  std::pair<Word, Cycle> getData(const Word idx) {
    if (idx & 3)
      throw std::runtime_error("unaligned memory access");
    if (cache)
      return cache.value()->getData(idx);
    return mainMemory->getData(idx);
  }

  Cycle writeData(const Word idx, const Word val) {
    if (idx & 3)
      throw std::runtime_error("unaligned memory access");
    if (program_begin <= idx and idx < program_end)
      std::cerr << "WARNING: write to program memory, may make program ill-formed\n";
    if (cache)
      return cache.value()->writeData(idx, val);
    return mainMemory->writeData(idx, val);
  }

  // UNSAFE fn to write to main memory directly, cache MUST NOT be used before
  // this should be used ONLY to initialize program in memory at beginning
  Cycle writeDataToMainMemory(const Word idx, const Word val) {
    return mainMemory->writeData(idx, val);
  }

  void dump(std::ostream &os) {
    if (cache) {
      cache.value()->dump(os);
      os << "\n";
    }
    mainMemory->dump(os);
  }
};

#endif /* end of __MEMORY_H */
