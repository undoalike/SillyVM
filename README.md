# SillyVM

SillyVM is a silly virtual machine implementation written in C. It features a barely useful minimal set architecture (ISA) with some basic arithmetic operations.

The interesting thing about it is dynamic opcode decoding - executing each instruction requires the decoder to descramble the opcode first. The descrambling is performed based on the opcode value as well as the current values in special registers rx and ry.
