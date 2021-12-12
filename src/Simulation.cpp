#include "Simulation.hpp"
#include <fstream> // for reading binary

// The following is necessary to be aligned, but clang-format breaks it.
// clang-format off

enum class INST_MASKS : Instruction {
  // The RISC-V ISA keeps the source (rs1 and rs2) and destination (rd)
  // registers at the same position in all formats to simplify decoding.
  rs2      = 0b00000001111100000000000000000000,
  rs1      = 0b00000000000011111000000000000000,
  rd       = 0b00000000000000000000111110000000,
  opcode   = 0b00000000000000000000000001111111,
  // R-type
  R_funct7 = 0b11111110000000000000000000000000,
  R_funct3 = 0b00000000000000000111000000000000,
  // I-Type
  I_imm    = 0b11111111111100000000000000000000,
  I_funct3 = 0b00000000000000000111000000000000,
  // S-type
  S_imm1   = 0b11111110000000000000000000000000,
  S_funct3 = 0b00000000000000000111000000000000,
  S_imm2   = 0b00000000000000000000111110000000,
  // B-type
  B_imm1   = 0b10000000000000000000000000000000,
  B_imm3   = 0b01111110000000000000000000000000,
  B_funct3 = 0b00000000000000000111000000000000,
  B_imm4   = 0b00000000000000000000111100000000,
  B_imm2   = 0b00000000000000000000000010000000,
  // U-type
  U_imm    = 0b11111111111111111111000000000000,
  // J-type
  J_imm1   = 0b10000000000000000000000000000000,
  J_imm4   = 0b01111111111000000000000000000000,
  J_imm3   = 0b00000000000100000000000000000000,
  J_imm2   = 0b00000000000011111111000000000000
};

enum class INST_OFFSETS : int {
  rs2      = 20,
  rs1      = 15,
  rd       = 7,
  opcode   = 0,
  // R-type
  R_funct7 = 25,
  R_funct3 = 12,
  // I-Type
  I_imm    = 20,
  I_funct3 = 12,
  // S-type
  S_imm1   = 25,
  S_funct3 = 12,
  S_imm2   = 7,
  // B-type
  B_imm1   = 31,
  B_imm3   = 25,
  B_funct3 = 12,
  B_imm4   = 8,
  B_imm2   = 7,
  // U-type
  U_imm    = 12,
  // J-type
  J_imm1   = 31,
  J_imm4   = 21,
  J_imm3   = 20,
  J_imm2   = 12
};

#define INST_GET(inst, type) \
  ((inst & static_cast<Instruction>(INST_MASKS::type)) >> static_cast<int>(INST_OFFSETS::type))

enum class INST_VALUES : Instruction {
  // R-type
  R_opcode         = 0x33,
  R_funct3_ADD_SUB = 0x0,
  R_funct7_ADD     = 0x00,
  R_funct7_SUB     = 0x20,
  R_funct3_SLL     = 0x1,
  R_funct7_SLL     = 0x00,
  R_funct3_XOR     = 0x4,
  R_funct7_XOR     = 0x00,
  R_funct3_SRA     = 0x5,
  R_funct7_SRA     = 0x20,
  R_funct3_OR      = 0x6,
  R_funct7_OR      = 0x00,
  R_funct3_AND     = 0x7,
  R_funct7_AND     = 0x00,
  // I-type
  I_opcode_load    = 0x03,
  I_funct3_LW      = 0x2,
  I_opcode_ADDI    = 0x13,
  I_funct3_ADDI    = 0x0,
  I_opcode_JALR    = 0x67,
  I_funct3_JALR    = 0x0,
  // S-type
  S_opcode         = 0x23,
  S_funct3_SW      = 0x2,
  // B-type
  B_opcode         = 0x63,
  B_funct3_BEQ     = 0x0,
  B_funct3_BNE     = 0x1,
  B_funct3_BLT     = 0x4,
  B_funct3_BGE     = 0x5,
  // U-type
  U_opcode_LUI     = 0x37,
  // J-type
  J_opcode_JAL     = 0x6f
};

// clang-format on

Word Simulation::initialize() {
  std::ifstream file(binary_path);
  if (!file)
    throw std::runtime_error("File '" + binary_path + "' does not exist!");

  Word idx = 0;
  for (std::string line; std::getline(file, line); idx += ILEN / 8) {
    if (line.length() != ILEN)
      throw std::runtime_error("Binary file is not of the correct format!");
    Instruction inst = 0;
    for (int i = 0; i < ILEN; ++i)
      inst = (inst << 1) | (line[i] == '1');
    memory.writeDataToMainMemory(idx, inst);
  }
  // tell memory subsytem the program memory address range
  memory.set_program_memory(0, idx);
  // inform caller about program memory address range end
  return idx;
}

void Simulation::simulate() {
  Word end = initialize();

  Cycle time = 0;

  for (Word PC = 0; PC != end; ) {
    // dump PC
    std::cout << "Program Counter : 0x" << std::hex << PC << std::dec << "\n";

    auto [inst, t_fetch] = memory.getData(PC);

    auto [new_PC, t_execute] = execute(inst, PC);

    // dump registers
    RF.dump(std::cout);
    // dump timing
    std::cout << "Time taken : " << t_fetch + t_execute << "\n\n";

    PC = new_PC;
    time += t_fetch + t_execute;
  }

  std::cout << "Total simulation cycles : " << time << "\n\n";
  memory.dump(std::cout);
}

std::pair<Word, Cycle> Simulation::execute(const Instruction I, Word PC) {
  Cycle t = 0;

  // destination register, initialized with improbable value to know if inst
  // doesn't have one
  unsigned rd_idx = no_of_registers;
  // value to be written to destination in Writeback stage
  Word result;
  // not all of these will be used, but these need to be calculated in Decode stage
  Word rs1, rs2, imm;

  // DECODE
  auto sext = [](Word x, int width) {
    Word mask = 1u << (width - 1); // mask with only <width>th bit set
    if (x & mask)                  // check if highest (sign) bit is set
      x |= ~(mask - 1); // set all bits other than last <width-1> bits
    return x;
  };
  switch (static_cast<INST_VALUES>(INST_GET(I, opcode))) {
  case INST_VALUES::I_opcode_load:
  case INST_VALUES::I_opcode_ADDI:
  case INST_VALUES::I_opcode_JALR: {
    rs1 = RF.getReg(INST_GET(I, rs1));
    rd_idx = INST_GET(I, rd);
    imm = sext(INST_GET(I, I_imm), 12);
  } break;

  case INST_VALUES::S_opcode: {
    rs1 = RF.getReg(INST_GET(I, rs1));
    rs2 = RF.getReg(INST_GET(I, rs2));
    imm = sext(INST_GET(I, S_imm1) << 5 | INST_GET(I, S_imm2), 12);
  } break;

  case INST_VALUES::R_opcode: {
    rs1 = RF.getReg(INST_GET(I, rs1));
    rs2 = RF.getReg(INST_GET(I, rs2));
    rd_idx = INST_GET(I, rd);
  } break;

  case INST_VALUES::U_opcode_LUI: {
    rd_idx = INST_GET(I, rd);
    imm = INST_GET(I, U_imm) << 12;
  } break;

  case INST_VALUES::B_opcode: {
    rs1 = RF.getReg(INST_GET(I, rs1));
    rs2 = RF.getReg(INST_GET(I, rs2));
    imm = INST_GET(I, B_imm1) << 12 | INST_GET(I, B_imm2) << 11 |
          INST_GET(I, B_imm3) << 5  | INST_GET(I, B_imm4) << 1;
    imm = sext(imm, 13);
  } break;

  case INST_VALUES::J_opcode_JAL: {
    rd_idx = INST_GET(I, rd);
    imm = INST_GET(I, J_imm1) << 20 | INST_GET(I, J_imm2) << 12 |
          INST_GET(I, J_imm3) << 11 | INST_GET(I, J_imm4) << 1;
    imm = sext(imm, 21);
  } break;

  default:
    throw std::runtime_error("invalid/unimplemented opcode");
  }
  // Decode takes 1 cycle as per project documentation
  t += 1;

  // EXECUTE
  switch (static_cast<INST_VALUES>(INST_GET(I, opcode))) {
  case INST_VALUES::I_opcode_load: {
    // LW
    if (static_cast<INST_VALUES>(INST_GET(I, I_funct3)) == INST_VALUES::I_funct3_LW) {
      auto [r_, t_] = memory.getData(rs1 + imm);
      result = r_;
      t += t_;
    } else {
      throw std::runtime_error("invalid/unimplemented instruction");
    }

    // standard increment as PC is unaffected
    PC += 4;
  } break;

  case INST_VALUES::I_opcode_ADDI: {
    // ADDI
    if (static_cast<INST_VALUES>(INST_GET(I, I_funct3)) == INST_VALUES::I_funct3_ADDI)
      result = rs1 + imm;
    else
      throw std::runtime_error("invalid/unimplemented instruction");

    // standard increment as PC is unaffected
    PC += 4;
  } break;

  case INST_VALUES::S_opcode: {
    // SW
    if (static_cast<INST_VALUES>(INST_GET(I, S_funct3)) == INST_VALUES::S_funct3_SW)
      t += memory.writeData(rs1 + imm, rs2);
    else
      throw std::runtime_error("invalid/unimplemented instruction");

    // standard increment as PC is unaffected
    PC += 4;
  } break;

  case INST_VALUES::R_opcode: {
    switch (static_cast<INST_VALUES>(INST_GET(I, R_funct3))) {
    case INST_VALUES::R_funct3_ADD_SUB: {
      switch (static_cast<INST_VALUES>(INST_GET(I, R_funct7))) {
      // ADD
      case INST_VALUES::R_funct7_ADD: {
        result = rs1 + rs2;
      } break;

      // SUB
      case INST_VALUES::R_funct7_SUB: {
        result = rs1 - rs2;
      } break;

      default:
        throw std::runtime_error("invalid/unimplemented instruction");
      }
    } break;

    case INST_VALUES::R_funct3_SLL: {
      // SLL
      if (static_cast<INST_VALUES>(INST_GET(I, R_funct7)) == INST_VALUES::R_funct7_SLL)
        result = rs1 << (rs2 & 0b11111);
      else
        throw std::runtime_error("invalid/unimplemented instruction");
    } break;

    case INST_VALUES::R_funct3_XOR: {
      // XOR
      if (static_cast<INST_VALUES>(INST_GET(I, R_funct7)) == INST_VALUES::R_funct7_XOR)
        result = rs1 ^ rs2;
      else
        throw std::runtime_error("invalid/unimplemented instruction");
    } break;

    case INST_VALUES::R_funct3_SRA: {
      // SRA
      if (static_cast<INST_VALUES>(INST_GET(I, R_funct7)) == INST_VALUES::R_funct7_SRA)
        result = static_cast<Word>(static_cast<SignedWord>(rs1) >> (rs2 & 0b11111));
      else
        throw std::runtime_error("invalid/unimplemented instruction");
    } break;

    case INST_VALUES::R_funct3_OR: {
      // OR
      if (static_cast<INST_VALUES>(INST_GET(I, R_funct7)) == INST_VALUES::R_funct7_OR)
        result = rs1 | rs2;
      else
        throw std::runtime_error("invalid/unimplemented instruction");
    } break;

    case INST_VALUES::R_funct3_AND: {
      // AND
      if (static_cast<INST_VALUES>(INST_GET(I, R_funct7)) == INST_VALUES::R_funct7_AND)
        result = rs1 & rs2;
      else
        throw std::runtime_error("invalid/unimplemented instruction");
    } break;

    default:
      throw std::runtime_error("invalid/unimplemented instruction");
    }

    // standard increment as PC is unaffected
    PC += 4;
  } break;

  case INST_VALUES::U_opcode_LUI: {
    // LUI
    result = imm;
    // standard increment as PC is unaffected
    PC += 4;
  } break;

  case INST_VALUES::B_opcode: {
    switch (static_cast<INST_VALUES>(INST_GET(I, B_funct3))) {
    // BEQ
    case INST_VALUES::B_funct3_BEQ: {
      if (rs1 == rs2)
        PC += imm;
      else
        PC += 4;
    } break;

    // BNE
    case INST_VALUES::B_funct3_BNE: {
      if (rs1 != rs2)
        PC += imm;
      else
        PC += 4;
    } break;

    // BLT
    case INST_VALUES::B_funct3_BLT: {
      if (static_cast<SignedWord>(rs1) < static_cast<SignedWord>(rs2))
        PC += imm;
      else
        PC += 4;
    } break;

    // BGE
    case INST_VALUES::B_funct3_BGE: {
      if (static_cast<SignedWord>(rs1) >= static_cast<SignedWord>(rs2))
        PC += imm;
      else
        PC += 4;
    } break;

    default:
      throw std::runtime_error("invalid/unimplemented instruction");
    }
  } break;

  case INST_VALUES::I_opcode_JALR: {
    // JALR
    if (static_cast<INST_VALUES>(INST_GET(I, I_funct3)) == INST_VALUES::I_funct3_JALR) {
      result = PC + 4;
      PC = (rs1 + imm) & ~1u;
    } else {
      throw std::runtime_error("invalid/unimplemented instruction");
    }
  } break;

  case INST_VALUES::J_opcode_JAL: {
    // JAL
    result = PC + 4;
    PC += imm;
  } break;

  default:
    throw std::runtime_error("impossible situation");
  }
  // Execute takes 1 cycle as per project documentation
  t += 1;

  // WRITEBACK
  if (rd_idx != no_of_registers) {
    RF.writeReg(rd_idx, result);
    // Writeback takes 1 cycle as per project documentation
    t += 1;
  }

  return {PC, t};
}
