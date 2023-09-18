_start:
mov r1, 0
mov r2, 1
mov r3, 5

loop:
    add r1, r1, r2
    cmp_neq r4, r3, r1
    jump_cond r4, loop
exit:
    mov r0, 0
    syscall


