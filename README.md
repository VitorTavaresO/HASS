# HASS - Hypothetical Architecture Simulator System

2 Simulator were created, one is a monocycle simulator and the other is a pipeline simulator

This is a Simulator for a Hypothetical Architecture with the following instruction set:

Format R
 - opcode 00 - add
 - opcode 01 - sub
 - opcode 02 - mul
 - opcode 03 - div
 - opcode 04 - cmp_equal
 - opcode 05 - cmp_neq
 - opcode 15 - load
 - opcode 16 - store
 - opcode 63 - syscall - Exit (Mov r0, 0)

 Format I:
 - opcode 00 - jump
 - opcode 01 - jump_cond
 - opcode 02 - empty
 - opcode 03 - mov
