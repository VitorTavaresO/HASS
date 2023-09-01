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
};

uint16_t search(struct Cpu cpu)
{
	return memory[cpu.addr];
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

uint16_t add(uint16_t a, uint16_t b)
{
	return a + b;
}

uint16_t sub(uint16_t a, uint16_t b)
{
	return a - b;
}

uint16_t mul(uint16_t a, uint16_t b)
{
	return a * b;
}

uint16_t divi(uint16_t a, uint16_t b)
{
	return a / b;
}

uint16_t cmp_equal(uint16_t a, uint16_t b)
{
	return (a == b);
}

uint16_t cmp_nequal(uint16_t a, uint16_t b)
{
	return (a != b);
}

uint16_t mov(uint16_t a)
{
	return a;
}

void execute(struct Cpu *cpu)
{
	switch (cpu->format)
	{
	case 0:
		switch (cpu->opcode)
		{
		case 0:
			registers[cpu->destiny] = add(registers[cpu->operator01], registers[cpu->operator02]);
			break;
		case 1:
			registers[cpu->destiny] = sub(registers[cpu->operator01], registers[cpu->operator02]);
			break;
		case 2:
			registers[cpu->destiny] = mul(registers[cpu->operator01], registers[cpu->operator02]);
			break;
		case 3:
			registers[cpu->destiny] = divi(registers[cpu->operator01], registers[cpu->operator02]);
			break;
		case 4:
			registers[cpu->destiny] = cmp_equal(registers[cpu->operator01], registers[cpu->operator02]);
			break;
		case 5:
			registers[cpu->destiny] = cmp_nequal(registers[cpu->operator01], registers[cpu->operator02]);
			break;
		}
		break;
	case 1:
		registers[cpu->destiny] = mov(cpu->operator01);
		break;
	}
}

int main(int argc, char **argv)
{

	memory[0] = 0b1000010001100100;
	memory[1] = 0b1001100000111001;
	memory[2] = 0b0000000101110001;
	struct Cpu cpu;
	cpu.addr = 0;
	cpu.instruction = search(cpu);
	printf("%d\n", cpu.instruction);
	decode(&cpu);
	printf("%d\n%d\n%d\n%d\n%d\n", cpu.format, cpu.opcode, cpu.destiny, cpu.operator01, cpu.operator02);
	execute(&cpu);
	printf("%d\n", registers[1]);
	return 0;
}
