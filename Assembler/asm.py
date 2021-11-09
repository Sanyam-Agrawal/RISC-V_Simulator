
from instruction import *

"""
Description:
	A 2-pass assembler for RISC-V ISA which takes the assebmly
	instruction as input(stdin) and produces the corresponding
	machine code(stdout).
"""
class Assembler:

	initialAssemblyInstructions     = []                     # in string format
	assemblyInstructionsAfterPass1  = []                     # as Instruction object
	binaryInstructions              = []                     # final output

	# dictionary of (key, value) = (label, line number containing label definition)
	labelsAddressMap                = {}
	InstObjectToOpcodeMap           = {
                                        R_Inst:   ["add", "sub", "and", "or", "xor", "sll", "sra"],
                                        I_Inst:   ["addi", "lw", "jalr"],
                                        S_Inst:   ["sw"],
                                        SB_Inst:  ["beq", "bne", "blt", "bge"],
                                        U_Inst:   ["lui"],
                                        UJ_Inst:  ["jal"],
                    				  }

	def __init__(self):
		self.readAssembly()


	"""
	return the correct object out of 6 instruction-formats classes
	depending on the opcode of instruction
	"""
	def getInstructionObject(self, instruction):

		firstWhitespace = instruction.index(' ')
		opcode = instruction[:firstWhitespace].lower()

		for (instructionObject, listOfOpcodes) in self.InstObjectToOpcodeMap.items():
			if opcode in listOfOpcodes:
				# Note: a lot work has happened in the constructor of corresponding class
				return instructionObject(instruction)

		raise Exception("Unknown Opcode:\t" + instruction)


	"""
	- forms labelsAddressMap which is used in pass2 to find 
	  offset for branch instructions and jal
	- separates label(if any) from instruction
	- allocate corresponding instruction format to each 
	  instruction and creates respective object
	"""
	def performPass1(self):

		for (instructionCounter, instruction) in enumerate(self.initialAssemblyInstructions):

			if self.hasLabel(instruction):
				label, instruction = self.extractLabel(instruction)
				self.labelsAddressMap[label] = instructionCounter        # keep line number labelsAddressMap

			instructionObject = self.getInstructionObject(instruction)
			self.assemblyInstructionsAfterPass1.append(instructionObject)


	"""
	in pass2, we need to find offset for branch and jalr instruction
	and finally store binary instruction of given assebmly instructions
	as list of 32-bit binary strings in self.binaryInstructions
	"""
	def performPass2(self):

		for (instructionCounter,instructionObject) in enumerate(self.assemblyInstructionsAfterPass1):

			if isinstance(instructionObject, (SB_Inst, UJ_Inst)):
				instructionObject.findOffset(instructionCounter, self.labelsAddressMap)

			binaryInstruction = instructionObject.getBinaryInstruction()
			self.binaryInstructions.append(binaryInstruction)


	def hasLabel(self, instruction):
		return ":" in instruction


	def extractLabel(self, instruction):

		colonIdx = instruction.index(":")

		label = instruction[:colonIdx]
		instruction = instruction[colonIdx + 2: ]

		return (label, instruction)


	"""
	read assembly instructions from stdin and store them in
	initialAssemblyInstructions as list of strings
	"""
	def readAssembly(self):

		try:
			while True:
				instruction = input()

				"""
				discard comments if any in given instruction
				comment in assembly instruction would be like in python i.e. starts with #
				"""
				indexOfCommentStart = instruction.find('#')

				if indexOfCommentStart != -1:
					instruction = instruction[:indexOfCommentStart]

				instruction = instruction.rstrip()   # remove whitespace from right side of instruction

				if len(instruction) != 0:            # discard blank line
					self.initialAssemblyInstructions.append(instruction)

		except:
			debug("Printing Actual Assembly Instructions")
			for instruction in self.initialAssemblyInstructions:
				debug(instruction)


	def printBinary(self):

		for binaryInstruction in self.binaryInstructions:
			print(binaryInstruction)


	def runAssembler(self):

		self.performPass1()
		self.performPass2()


if __name__ == "__main__":

	a = Assembler()
	a.runAssembler()
	a.printBinary()
