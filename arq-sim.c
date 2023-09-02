#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include "lib.h"
#include "lib.c"

#define MEMORY 64 * 1024
#define REGISTERS 8

uint16_t memory[MEMORY];
uint16_t registers[REGISTERS];

// Struct com as inforamções
struct Cpu
{
	uint16_t addr;
	uint16_t instruction;
	uint16_t format;
	uint16_t opcode;
	uint16_t destiny;
	uint16_t operator01;
	uint16_t operator02;
	bool alive;
} cpu;

// Busca as instruções na memória
void search(struct Cpu *cpu)
{
	cpu->instruction = memory[cpu->addr];
}

// Decodifica as instruções
void decode(struct Cpu *cpu)
{
	cpu->format = extract_bits(cpu->instruction, 15, 1);
	switch (cpu->format)
	{
	case 0: // Padrão de instruções R
		cpu->opcode = extract_bits(cpu->instruction, 9, 6);
		cpu->destiny = extract_bits(cpu->instruction, 6, 3);
		cpu->operator01 = extract_bits(cpu->instruction, 3, 3);
		cpu->operator02 = extract_bits(cpu->instruction, 0, 3);
		break;
	case 1: // Padrão de instruções I
		cpu->opcode = extract_bits(cpu->instruction, 13, 2);
		cpu->destiny = extract_bits(cpu->instruction, 10, 3);
		cpu->operator01 = extract_bits(cpu->instruction, 0, 10);
		cpu->operator02 = 0;
		break;
	}
}

void execute(struct Cpu *cpu)
{
	switch (cpu->format)
	{
	case 0: // Instruções R
		switch (cpu->opcode)
		{
		case 0: // add
			registers[cpu->destiny] = registers[cpu->operator01] + registers[cpu->operator02];
			break;
		case 1: // sub
			registers[cpu->destiny] = registers[cpu->operator01] - registers[cpu->operator02];
			break;
		case 2: // mul
			registers[cpu->destiny] = registers[cpu->operator01] * registers[cpu->operator02];
			break;
		case 3: // div
			registers[cpu->destiny] = registers[cpu->operator01] / registers[cpu->operator02];
			break;
		case 4: // cmp_eq
			registers[cpu->destiny] = registers[cpu->operator01] == registers[cpu->operator02];
			break;
		case 5: // cmp_nq
			registers[cpu->destiny] = registers[cpu->operator01] != registers[cpu->operator02];
			break;
		case 6: // load
			registers[cpu->destiny] = memory[registers[cpu->operator01]];
			break;
		case 7: // store
			memory[registers[cpu->destiny]] = registers[cpu->operator01];
			break;
		case 63: // exit
			cpu->alive = false;
			break;
		}
		break;
	case 1: // Instruções I
		switch (cpu->opcode)
		{
		case 0: // jump
			cpu->addr = (cpu->operator01 - 1);
			break;
		case 1: // jump_cond
			switch (cpu->destiny)
			{
			case 0:
				break;
			case 1:
				cpu->addr = (cpu->operator01 - 1);
				break;
			}
			break;
		case 2: // empty
			break;

		case 3: // mov
			registers[cpu->destiny] = cpu->operator01;
			break;
		}
		break;
	}
}

int main(int argc, char **argv)
{
	memory[0] = 0b1000000000000010;	 // jump
	memory[1] = 0b0111111000000000;	 // exit syscall
	memory[2] = 0b1110010001100100;	 // mov r1, 100
	memory[3] = 0b1111100000111001;	 // mov r6, 57
	memory[4] = 0b0000000101110001;	 // add r5, r6, r1
	memory[5] = 0b1111110000000000;	 // mov r7, 0
	memory[6] = 0b0000111111101000;	 // store (r7), r5
	memory[7] = 0b0000110100111000;	 // load r4, (r7)
	memory[8] = 0b0000001100100110;	 // sub r4, r4, r6
	memory[9] = 0b0000100011101001;	 // cmp_eq r3, r4, r1
	memory[10] = 0b1010110000000001; // jump_cond r3, 1

	cpu.addr = 0;
	cpu.alive = true;
	while (cpu.alive)
	{
		search(&cpu);
		decode(&cpu);
		execute(&cpu);
		cpu.addr++;
	}
	printf("%d\n", registers[4]);
	return 0;
}
