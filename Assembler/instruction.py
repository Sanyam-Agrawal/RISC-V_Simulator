
from pprint import pprint

SizeOfInstruction = 4            # in bytes
DEBUG = False

def debug(msg, blankline=False):

	if DEBUG:
		if blankline:
			print()
		else:
			pprint(msg, indent=4)

"""
There are 6 formats of instructions in RISC-V and each instruction
belongs to one of the 6 formats.

Design: 
		Instruction is base class of 6 classes, implementing each
		of the 6 intruction formats of RISC-V.
		
		6 derived classes of Instruction class:
			- R_Inst
			- I_Inst
			- S_Inst
			- SB_Inst
			- U_Inst
			- UJ_Inst
"""
class Instruction:

	instruction                     = ""                    # instruction as string
	tokensOfInstruction             = []                    # instruction.split(' ')
	opcode                          = ""                    # like addi, lw, sw...

	opcodeInBinary                  = ""                    # last 7 bits of binaryInstruction
	binaryInstruction               = ""                    # final 32 bit binary Instruction as string

	# dictOfFields: only for debugging purpose
	# 	(key, value) = (field, binary value of field)
	# 	example:
	# 			{
	# 				"immediate_7": {0,1}^7,
	# 				"funct3": {0,1}^3,
	# 				...
	# 			}
	dictOfFields                    = {}


	def __init__(self, instruction):

		self.instruction           = instruction
		self.instruction = self.instruction.rstrip()   # remove whitespace from right side of instruction
		self.tokensOfInstruction   = self.instruction.split()
		self.opcode                = self.tokensOfInstruction[0].lower()
		self.opcodeInBinary        = self.getOpcodeInBinary()


	# returns 7-bit opcode as string depending on the instruction's opcode
	def getOpcodeInBinary(self):
		raise NotImplementedError("Must override getOpcodeInBinary()")


	"""
	- Returns exactly 5-bit binary encoding for each register r0-r31
	- maps ra to r1
	- regex format of variable register: a string, "r|R{0-31}|a"
	- Zero bit extended for r0-r15
	- It doesn't care about case of register i.e. R13 == r13
	- It doesn't handle any errors like if register = s12, r342, r-21
	"""
	def getRegisterInBinary(self, register, requiredLength=5):
		# register[1:] - to ignore 'r/R'
		# [2:] - bin returns in "0b..." format so ignoring first two characters
		register = register[1:]

		if register == 'a':                      # ra mapped to r1
			register = '1'

		if not (0 <= int(register) <= 31):
			raise Exception("Unknown register: " + self.instruction)

		binaryEncoding = bin(int(register))[2:]
		return "0" * (requiredLength - len(binaryEncoding)) + binaryEncoding     # Zero bit extended


	"""
	- Returns binary encoding of offset/immediate
	- Uses 2'complement and so MSB bit extended
	- returns exactly "requiredLength" number of bits
	"""
	def getImmediateInBinary(self, immediate, requiredLength):

		immediate  = int(immediate)

		# if immediate >= 2**(requiredLength - 1) or immediate < -2**(requiredLength - 1) :
		# 	raise Exception("Immeditate value overflow: " + self.instruction)

		isNegative = immediate < 0
		immediate  = abs(immediate)

		binaryEncoding = bin(immediate)[2:]

		binaryEncoding = "0" * (requiredLength - len(binaryEncoding)) \
						 + binaryEncoding                                # Zero-bit extended

		if isNegative:
			# convert to 2's complement form using:
			# https://stackoverflow.com/questions/34982608/why-does-this-twos-complement-shortcut-work
			lastIndexOfOne = binaryEncoding.rindex('1')
			binaryEncoding = ''.join([ str(1 ^ int(i)) for i in binaryEncoding[:lastIndexOfOne] ])   \
							 + binaryEncoding[lastIndexOfOne:]

		length = len(binaryEncoding)
		return binaryEncoding[length - requiredLength:length]


	"""
	separates offset and source register for store/load instruction
	i.e. offset(rs1) to rs1, offset in tokensOfInstruction
	idx = index in tokensOfInstruction where this special format is there

		Example: 
		Instruction         = sw r14 8(r2)
		tokensOfInstruction = ["sw", "r14", "8(r2)"]
		We want tokensOfInstruction = ["sw", "r14", "r2", "8"]
	"""
	def separateOffsetFromSourceRegister(self, idx):

		if self.opcode in ["sw", "lw"]:
			# registerWithOffset = "offset(rs1)""
			registerWithOffset = self.tokensOfInstruction[idx]
			length = len(registerWithOffset)

			self.tokensOfInstruction.pop()

			bracketIdx = registerWithOffset.index('(')
			immediate = registerWithOffset[:bracketIdx]
			register = registerWithOffset[bracketIdx + 1: length - 1]

			self.tokensOfInstruction.extend([register, immediate])

		else:
			pass


	def getBinaryInstruction(self):
		return self.binaryInstruction


	# For debugging only
	def debugInstruction(self):
		debug("", blankline=True)
		debug(self.instruction)
		debug(self.tokensOfInstruction)
		debug(self.dictOfFields)
		debug(self.binaryInstruction)

		return



"""
Parent class: Instruction
Instruction of R-format type
	- R-format type: instructions using 3 register inputs
	- deals with opcodes: ["add", "sub", "and", "or", "xor", "sll", "sra"]
Example: 			opcode rd rs1 rs2
binaryInstruction:	funct7 | rs2 | rs1 | funct3 | rd | opcode
"""
class R_Inst(Instruction):

	funct7 	= ""
	funct3 	= ""
	rd      = ""
	rs1     = ""
	rs2     = ""

	def __init__(self, instruction):

		super().__init__(instruction)

		self.rd     = self.getRegisterInBinary(self.tokensOfInstruction[1])
		self.rs1    = self.getRegisterInBinary(self.tokensOfInstruction[2])
		self.rs2    = self.getRegisterInBinary(self.tokensOfInstruction[3])
		self.funct3 = self.getFunct3()
		self.funct7 = self.getFunct7()

		self.binaryInstruction = self.funct7 + self.rs2 + self.rs1 \
								+ self.funct3 + self.rd + self.opcodeInBinary

		# For debugging only
		self.dictOfFields = {
							 "funct7":self.funct7                  ,
							 "rs2":self.rs2                        ,
							 "rs1":self.rs1                        ,
							 "funct3":self.funct3                  ,
							 "rd":self.rd                          ,
							 "opcodeInBinary":self.opcodeInBinary  ,
							 }
		self.debugInstruction()


	def getOpcodeInBinary(self):
		return "0110011"								# add, sub, and, or, xor, sll, sra: all have same opcode


	def getFunct3(self):

		if self.opcode in ["add", "sub"]:
			return "000"
		elif self.opcode == "and":
			return "111"
		elif self.opcode == "or":
			return "110"
		elif self.opcode == "xor":
			return "100"
		elif self.opcode == "sll":
			return "001"
		elif self.opcode == "sra":
			return "101"
		else:
			raise Exception("Unknown Opcode: " + self.instruction)


	def getFunct7(self):
		if self.opcode in ["add", "sll", "xor", "or", "and"]:
			return "0" * 7
		else:
			return "0100000"                            # sub, sra

"""
Parent class: Instruction
Instruction of I-format type
	- I-format type: instructions with immediates and load
	- deals with opcodes: ["addi", "lw", "jalr"]
Example: 			opcode rd rs1 immediate
binaryInstruction:	immediate | rs1 | funct3 | rd | opcode
"""
class I_Inst(Instruction):

	funct3    = ""
	rd        = ""
	rs1       = ""
	immediate = ""                                # 12 bits

	def __init__(self, instruction):

		super().__init__(instruction)

		self.separateOffsetFromSourceRegister(-1)        # for lw

		self.rd         = self.getRegisterInBinary(self.tokensOfInstruction[1])
		self.rs1        = self.getRegisterInBinary(self.tokensOfInstruction[2])
		self.funct3     = self.getFunct3()
		self.immediate  = self.getImmediateInBinary(self.tokensOfInstruction[3], 12)

		self.binaryInstruction = self.immediate + self.rs1 + self.funct3       \
								 + self.rd + self.opcodeInBinary

		# For debugging only
		self.dictOfFields = {
							 "immediate":self.immediate            ,
							 "rs1":self.rs1                        ,
							 "funct3":self.funct3                  ,
							 "rd":self.rd                          ,
							 "opcodeInBinary":self.opcodeInBinary  ,
							 }
		self.debugInstruction()


	def getOpcodeInBinary(self):
		if self.opcode == "addi":
			return "0010011"
		elif self.opcode == "lw":
			return "0000011"
		elif self.opcode == "jalr":
			return "1100111"
		else:
			raise Exception("Unknown Opcode: " + self.instruction)


	def getFunct3(self):
		if self.opcode in ["addi", "jalr"]:
			return "000"
		elif self.opcode == "lw":
			return "010"
		else:
			raise Exception("Unknown Opcode: " + self.instruction)



"""
Parent class: Instruction
Instruction of S-format type
	- S-format type: store instructions 
	- deals with opcodes: ["sw"]
Example: 			opcode rs2 immediate(rs1) 
binaryInstruction:	immediate_7 | rs2 | rs1 | funct3 | immediate_5 | opcode
"""
class S_Inst(Instruction):

	rs1         = ""
	rs2         = ""
	funct3      = ""
	immediate_5 = ""
	immediate_7 = ""

	def __init__(self, instruction):

		super().__init__(instruction)

		self.separateOffsetFromSourceRegister(-1)        # for sw

		self.rs1         = self.getRegisterInBinary(self.tokensOfInstruction[2])
		self.rs2         = self.getRegisterInBinary(self.tokensOfInstruction[1])
		self.funct3      = self.getFunct3()

		immediate        = self.getImmediateInBinary(self.tokensOfInstruction[3], 12)
		self.immediate_7 = immediate[:7]
		self.immediate_5 = immediate[7:]

		self.binaryInstruction = self.immediate_7 + self.rs2 + self.rs1 + self.funct3  \
								 + self.immediate_5 + self.opcodeInBinary

		# For debugging only
		self.dictOfFields = {
							 "immediate_7":self.immediate_7        ,
							 "rs2":self.rs2                        ,
							 "rs1":self.rs1                        ,
							 "funct3":self.funct3                  ,
							 "immediate_5":self.immediate_5        ,
							 "opcodeInBinary":self.opcodeInBinary  ,
							 }
		self.debugInstruction()


	def getOpcodeInBinary(self):
		if self.opcode == "sw":
			return "0100011"
		else:
			raise Exception("Unknown Opcode: " + self.instruction)


	def getFunct3(self):
		return "010"                                      # for sw


"""
Parent class: Instruction
Instruction of SB-format type
	- SB-format type: branch instructions 
	- deals with opcodes: ["beq", "bne", "blt", "bge"]
Example: 			opcode rs2 rs2 offset 
binaryInstruction:	immediate_7 | rs2 | rs1 | funct3 | immediate_5 | opcode
"""
class SB_Inst(Instruction):

	rs1         = ""
	rs2         = ""
	funct3      = ""
	immediate_5 = ""
	immediate_7 = ""

	def __init__(self, instruction):

		super().__init__(instruction)

		self.rs1    = self.getRegisterInBinary(self.tokensOfInstruction[1])
		self.rs2    = self.getRegisterInBinary(self.tokensOfInstruction[2])
		self.funct3 = self.getFunct3()

		# find rest fields in pass2 of assembler (when class method findOffset is called)


	def getOpcodeInBinary(self):
		return "1100011"                                # beq, bne, blt, bge


	def getFunct3(self):
		if self.opcode == "beq":
			return "000"
		elif self.opcode == "bne":
			return "001"
		elif self.opcode == "blt":
			return "100"
		elif self.opcode == "bge":
			return "101"
		else:
			raise Exception("Unknown Opcode: " + self.instruction)

	"""
	instructionLocationIdx: line number of this instruction in given assembly instructions
	labelsAddressMap: dictionary of (key, value) = (label, line number containing label definition)
	"""
	def findOffset(self, instructionLocationIdx, labelsAddressMap):

		# last element in tokensOfInstruction is label name which we have to replace with relative offset
		labelAddress = labelsAddressMap[self.tokensOfInstruction[-1]]
		offset = (labelAddress - instructionLocationIdx) * SizeOfInstruction
		self.tokensOfInstruction[-1] = str(offset)

		requiredLength = 13                             # we will ignore 1 bit so asking 1 extra
		immediate = self.getImmediateInBinary(self.tokensOfInstruction[-1], requiredLength)

		"""
		issue1: in the manual, LSB is index 0 while in python MSB is index 0
		issue2: in the manual string[a:b] means b inclusive while we need to add 1 in b
		 		for same effect

		Workaround for issue 1:
			reverse the immediate first and when taking some substring of this,
			reverse again
		Workaround for issue 2:
			simply add 1 when slicing, LOL
		"""
		immediate = immediate[::-1]
		self.immediate_5 = immediate[1:4 + 1][::-1] + immediate[11][::-1]
		self.immediate_7 = immediate[12][::-1] + immediate[5:10 + 1][::-1]

		self.binaryInstruction = self.immediate_7 + self.rs2 + self.rs1 + self.funct3 \
								 + self.immediate_5 + self.opcodeInBinary

		# For debugging only
		self.dictOfFields = {
							 "immediate_7":self.immediate_7        ,
							 "rs2":self.rs2                        ,
							 "rs1":self.rs1                        ,
							 "funct3":self.funct3                  ,
							 "immediate_5":self.immediate_5        ,
							 "opcodeInBinary":self.opcodeInBinary  ,
							 }
		self.debugInstruction()



"""
Parent class: Instruction
Instruction of U-format type
	- U-format type: instructions with upper immediates
	- deals with opcodes: ["lui"]
Example: 			opcode rd immediate 
binaryInstruction:	immediate | rd | opcode
"""
class U_Inst(Instruction):

	rd        = ""
	immediate = ""

	def __init__(self, instruction):

		super().__init__(instruction)

		self.rd        = self.getRegisterInBinary(self.tokensOfInstruction[1])
		self.immediate = self.getImmediateInBinary(self.tokensOfInstruction[2], 20)

		self.binaryInstruction = self.immediate + self.rd + self.opcodeInBinary

		# For debugging only
		self.dictOfFields = {
							 "immediate":self.immediate            ,
							 "rd":self.rd                          ,
							 "opcodeInBinary":self.opcodeInBinary  ,
							 }
		self.debugInstruction()


	def getOpcodeInBinary(self):
		return "0110111"                                # for lui

"""
Parent class: Instruction
Instruction of UJ-format type
	- UJ-format type: jump instruction- jal
	- deals with opcodes: ["jal"]
Example: 			opcode rd immediate 
binaryInstruction:	immediate | rd | opcode
"""
class UJ_Inst(Instruction):

	rd        = ""
	immediate = ""

	def __init__(self, instruction):

		super().__init__(instruction)

		self.rd = self.getRegisterInBinary(self.tokensOfInstruction[1])

		# find rest fields in pass2 of assembler(when class method findOffset is called)


	def getOpcodeInBinary(self):
		return "1101111"                                # jal


	"""
	instructionLocationIdx: line number of this instruction in given assembly instructions
	labelsAddressMap: dictionary of (key, value) = (label, line number containing label definition)
	"""
	def findOffset(self, instructionLocationIdx, labelsAddressMap):

		# last element in tokensOfInstruction is label name which we have to replace with relative offset
		labelAddress = labelsAddressMap[self.tokensOfInstruction[-1]]
		offset = (labelAddress - instructionLocationIdx) * SizeOfInstruction
		self.tokensOfInstruction[-1] = str(offset)

		requiredLength = 21                             # we will ignore 1 bit so asking 1 extra
		immediate = self.getImmediateInBinary(self.tokensOfInstruction[-1], requiredLength)
		"""
		issue1: in the manual, LSB is index 0 while in python MSB is index 0
		issue2: in the manual string[a:b] means b inclusive while we need to add 1 in b
		 		for same effect

		Workaround for issue 1:
			reverse the immediate first and when taking some substring of this,
			reverse again
		Workaround for issue 2:
			simply add 1 when slicing, LOL
		"""
		immediate = immediate[::-1]
		self.immediate = immediate[20][::-1] + immediate[1:10 + 1][::-1] \
						 + immediate[11][::-1] + immediate[12:19 + 1][::-1]

		self.binaryInstruction = self.immediate + self.rd + self.opcodeInBinary

		# For debugging only
		self.dictOfFields = {
							 "immediate":self.immediate            ,
							 "rd":self.rd                          ,
							 "opcodeInBinary":self.opcodeInBinary  ,
							 }
		self.debugInstruction()
