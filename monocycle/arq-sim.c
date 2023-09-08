#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include "lib.h"
#include "lib.c"

#define DEBUG

#ifdef DEBUG
#define dprint(...) printf(__VA_ARGS__)
#define dprintln(f, ...) printf(f "\n", __VA_ARGS__)
#else
#define dprint(...)
#define dprintln(f, ...)
#endif
#define MEMORY_SIZE 64 * 1024
#define REGISTERS 8

uint16_t memory[MEMORY_SIZE];
uint16_t registers[REGISTERS];

struct Cpu
{
	uint16_t pc;
	uint16_t instruction;
	uint16_t format;
	uint16_t opcode;
	uint16_t destiny;
	uint16_t operator01;
	uint16_t operator02;
	bool alive;
} cpu;

void print_registers()
{
	for (int i = 0; i < REGISTERS; i++)
	{
		printf("r%d:%d ", i, registers[i]);
	}
}

void print_200_memory()
{
	for (int i = 0; i < 200; i++)
	{
		printf("%d ", memory[i]);
	}
}

void search(struct Cpu *cpu)
{
	cpu->instruction = memory[cpu->pc];
	dprintln("pc: %d", cpu->pc);
	cpu->pc++;
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
		dprintln("formato R", cpu->format);
		switch (cpu->opcode)
		{
		case 0:
			registers[cpu->destiny] = registers[cpu->operator01] + registers[cpu->operator02];
			dprint("add r%d, r%d, r%d\n", cpu->destiny, cpu->operator01, cpu->operator02);
			break;
		case 1:
			registers[cpu->destiny] = registers[cpu->operator01] - registers[cpu->operator02];
			dprintln("sub r%d, r%d, r%d", cpu->destiny, cpu->operator01, cpu->operator02);
			break;
		case 2:
			registers[cpu->destiny] = registers[cpu->operator01] * registers[cpu->operator02];
			dprintln("mul r%d, r%d, r%d", cpu->destiny, cpu->operator01, cpu->operator02);
			break;
		case 3:
			registers[cpu->destiny] = registers[cpu->operator01] / registers[cpu->operator02];
			dprintln("div r%d, r%d, r%d", cpu->destiny, cpu->operator01, cpu->operator02);
			break;
		case 4:
			registers[cpu->destiny] = registers[cpu->operator01] == registers[cpu->operator02];
			dprintln("cmp_eq r%d, r%d, r%d", cpu->destiny, cpu->operator01, cpu->operator02);
			break;
		case 5:
			registers[cpu->destiny] = registers[cpu->operator01] != registers[cpu->operator02];
			dprintln("cmp_nq r%d, r%d, r%d", cpu->destiny, cpu->operator01, cpu->operator02);
			break;
		case 15:
			registers[cpu->destiny] = memory[registers[cpu->operator01]];
			dprintln("load r%d, (r%d)", cpu->destiny, cpu->operator01);
			break;
		case 16:
			memory[registers[cpu->operator01]] = registers[cpu->operator02];
			dprintln("store (r%d), r%d", cpu->operator01, cpu->operator02);
			break;
		case 63:
			if (registers[cpu->operator01] == 0)
			{
				cpu->alive = false;
				dprint("exit");
			}
			break;
		default:
			printf("Instrução não implementada\n");
			exit(1);
		}
		break;
	case 1:
		dprintln("formato I", cpu->format);
		switch (cpu->opcode)
		{
		case 0:
			cpu->pc = cpu->operator01;
			dprintln("jump %d\n", cpu->operator01);
			break;
		case 1:
			switch (registers[cpu->destiny])
			{
			case 0:
				dprint("jump_cond nao atendida");
				break;
			case 1:
				cpu->pc = cpu->operator01;
				dprint("jump_cond r%d, %d\n", cpu->destiny, cpu->operator01);
				break;
			}
			break;
		case 2:
			cpu->alive = false;
			printf("Instrução não implementada\n");
			break;

		case 3:
			registers[cpu->destiny] = cpu->operator01;
			dprintln("mov r%d, %d", cpu->destiny, cpu->operator01);
			break;
		default:
			printf("Instrução não implementada\n");
			exit(1);
		}
		break;
	default:
		printf("Instrução não implementada\n");
		exit(1);
	}
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf("usage: %s [bin_name]\n", argv[0]);
		exit(1);
	}

	load_binary_to_memory(argv[1], memory, MEMORY_SIZE * 2);

	cpu.pc = 1;
	cpu.alive = true;
	while (cpu.alive)
	{
		search(&cpu);
		decode(&cpu);
		execute(&cpu);
		dprint("-------------------\n");
		getchar();
	}
	printf("Fim da execucao\n");
	print_registers();
	printf("\n");
	print_200_memory();
	printf("\n");
	return 0;
}