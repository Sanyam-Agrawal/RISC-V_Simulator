# RISC-V Simulator

This project aims to simulate a certain subset of RISC-V ISA.

## Building

Run the following commands in the root directory of the project:

```bash
$ mkdir build
$ cd build
$ cmake ..
$ make
$ ./risc-v-sim <(python3 ../Assembler/asm.py < <test>)
```
