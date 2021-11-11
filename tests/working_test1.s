xor r6 r6 r6     
addi r6 r6 180
xor r12 r12 r12
xor r5 r5 r5
addi r5 r5 256
xor r2 r2 r2
addi r2 r2 9
loop1_start: bge r12 r6 loop1_end
or r3 r12 r2
sw r3 0(r5)
addi r5 r5 4
addi r12 r12 1
beq r12 r12 loop1_start
loop1_end: xor r31 r31 r31
lui r7 0
addi r7 r7 256
lui r12 0
lui r3 0
loop2_start: bge r12 r6 loop2_end
lw r4 0(r7)
lui r10 0
addi r10 r10 1
sll r4 r4 r10
lui r10 0
addi r10 r10 1
sub r4 r4 r10
add r3 r3 r4
addi r7 r7 4
addi r12 r12 1
jal r11 loop2_start
loop2_end: and r12 r12 r5
lui r13 -45
xor r14 r7 r12
add r15 r14 r13
sra r16 r15 r13