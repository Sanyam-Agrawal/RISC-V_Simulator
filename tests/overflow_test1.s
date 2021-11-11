xor r6 r6 r6     
addi r6 r6 -2048         # this is not overflow, r6 = -2048

xor r5 r5 r5     
addi r5 r5 2048          # this is overflow, r5 = -2048


xor r2 r2 r2     
addi r2 r2 -2049         # this is oveflow, r2 = 2047
