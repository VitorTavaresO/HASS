#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include "lib.h"

#define MEMORY 64 * 1024
#define REGISTERS 8

uint16_t memory[MEMORY];
uint16_t registers[REGISTERS];

struct Cpu
{
	uint16_t addr;
	uint16_t instruction;
	uint16_t format;
	uint16_t opcode;
	uint16_t destiny;
	uint16_t operator01;
	uint16_t operator02;
} cpu;

void search(struct Cpu *cpu)
{
	cpu->instruction = memory[cpu->addr];
}

void decode(struct Cpu *cpu)
{
	cpu->format = extract_bits(cpu->instruction, 15, 1);
	switch (cpu->format)
	{
	case 0:
		cpu->opcode = extract_bits(cpu->instruction, 9, 6);
		cpu->destiny = extract_bits(cpu->instruction, 6, 3);
		cpu->operator01 = extract_bits(cpu->instruction, 3, 3);
		cpu->operator02 = extract_bits(cpu->instruction, 0, 3);
		break;
	case 1:
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
	case 0:
		switch (cpu->opcode)
		{
		case 0:
			registers[cpu->destiny] = registers[cpu->operator01] + registers[cpu->operator02];
			break;
		case 1:
			registers[cpu->destiny] = registers[cpu->operator01] - registers[cpu->operator02];
			break;
		case 2:
			registers[cpu->destiny] = registers[cpu->operator01] * registers[cpu->operator02];
			break;
		case 3:
			registers[cpu->destiny] = registers[cpu->operator01] / registers[cpu->operator02];
			break;
		case 4:
			registers[cpu->destiny] = registers[cpu->operator01] == registers[cpu->operator02];
			break;
		case 5:
			registers[cpu->destiny] = registers[cpu->operator01] != registers[cpu->operator02];
			break;
		}
		break;
	case 1:
		switch (cpu->opcode)
		{
		case 0:
			registers[cpu->destiny] = cpu->operator01;
			break;
		}
		break;
	}
}

int main(int argc, char **argv)
{
	memory[0] = 0b1000010001100100;
	memory[1] = 0b1001100000111001;
	memory[2] = 0b0000000101110001;
	memory[3] = 0b0000110011010001;
	for (cpu.addr = 0; cpu.addr < 4; cpu.addr++)
	{
		search(&cpu);
		decode(&cpu);
		execute(&cpu);
	}
	printf("%d\n", registers[cpu.destiny]);
	return 0;
}
