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

struct searchStage
{
	uint16_t pc;
	uint16_t instruction;
} searchStage;

struct decodeStage
{
	uint16_t instruction;
	uint16_t format;
	uint16_t opcode;
	uint16_t destiny;
	uint16_t operator01;
	uint16_t operator02;
} decodeStage;

struct executeStage
{
	uint16_t format;
	uint16_t opcode;
	uint16_t destiny;
	uint16_t operator01;
	uint16_t operator02;
	bool alive;
} executeStage;

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

void search(struct searchStage *searchStage)
{
	searchStage->instruction = memory[searchStage->pc];
	dprintln("pc: %d", searchStage->pc);
	searchStage->pc++;
}

void decode(struct searchStage *searchStage, struct decodeStage *decodeStage)
{
	decodeStage->instruction = searchStage->instruction;
	decodeStage->format = extract_bits(decodeStage->instruction, 15, 1);
	switch (decodeStage->format)
	{
	case 0:
		decodeStage->opcode = extract_bits(decodeStage->instruction, 9, 6);
		decodeStage->destiny = extract_bits(decodeStage->instruction, 6, 3);
		decodeStage->operator01 = extract_bits(decodeStage->instruction, 3, 3);
		decodeStage->operator02 = extract_bits(decodeStage->instruction, 0, 3);
		break;
	case 1:
		decodeStage->opcode = extract_bits(decodeStage->instruction, 13, 2);
		decodeStage->destiny = extract_bits(decodeStage->instruction, 10, 3);
		decodeStage->operator01 = extract_bits(decodeStage->instruction, 0, 10);
		decodeStage->operator02 = 0;
		break;
	}
}

void execute(struct searchStage *searchStage, struct decodeStage *decodeStage, struct executeStage *executeStage)
{
	executeStage->format = decodeStage->format;
	executeStage->opcode = decodeStage->opcode;
	executeStage->destiny = decodeStage->destiny;
	executeStage->operator01 = decodeStage->operator01;
	executeStage->operator02 = decodeStage->operator02;
	switch (executeStage->format)
	{
	case 0:
		dprintln("formato R", executeStage->format);
		switch (executeStage->opcode)
		{
		case 0:
			registers[executeStage->destiny] = registers[executeStage->operator01] + registers[executeStage->operator02];
			dprint("add r%d, r%d, r%d\n", executeStage->destiny, executeStage->operator01, executeStage->operator02);
			break;
		case 1:
			registers[executeStage->destiny] = registers[executeStage->operator01] - registers[executeStage->operator02];
			dprintln("sub r%d, r%d, r%d", executeStage->destiny, executeStage->operator01, executeStage->operator02);
			break;
		case 2:
			registers[executeStage->destiny] = registers[executeStage->operator01] * registers[executeStage->operator02];
			dprintln("mul r%d, r%d, r%d", executeStage->destiny, executeStage->operator01, executeStage->operator02);
			break;
		case 3:
			registers[executeStage->destiny] = registers[executeStage->operator01] / registers[executeStage->operator02];
			dprintln("div r%d, r%d, r%d", executeStage->destiny, executeStage->operator01, executeStage->operator02);
			break;
		case 4:
			registers[executeStage->destiny] = registers[executeStage->operator01] == registers[executeStage->operator02];
			dprintln("cmp_eq r%d, r%d, r%d", executeStage->destiny, executeStage->operator01, executeStage->operator02);
			break;
		case 5:
			registers[executeStage->destiny] = registers[executeStage->operator01] != registers[executeStage->operator02];
			dprintln("cmp_nq r%d, r%d, r%d", executeStage->destiny, executeStage->operator01, executeStage->operator02);
			break;
		case 15:
			registers[executeStage->destiny] = memory[registers[executeStage->operator01]];
			dprintln("load r%d, (r%d)", executeStage->destiny, executeStage->operator01);
			break;
		case 16:
			memory[registers[executeStage->operator01]] = registers[executeStage->operator02];
			dprintln("store (r%d), r%d", executeStage->operator01, executeStage->operator02);
			break;
		case 63:
			if (registers[executeStage->operator01] == 0)
			{
				executeStage->alive = false;
				dprint("exit");
			}
			break;
		default:
			printf("Instrução não implementada\n");
			executeStage->alive = false;
		}
		break;
	case 1:
		dprintln("formato I", executeStage->format);
		switch (executeStage->opcode)
		{
		case 0:
			searchStage->pc = executeStage->operator01;
			dprintln("jump %d\n", executeStage->operator01);
			break;
		case 1:
			switch (registers[executeStage->destiny])
			{
			case 0:
				dprint("jump_cond nao atendida");
				break;
			case 1:
				searchStage->pc = executeStage->operator01;
				dprint("jump_cond r%d, %d\n", executeStage->destiny, executeStage->operator01);
				break;
			}
			break;
		case 2:
			executeStage->alive = false;
			printf("Instrução não implementada\n");
			break;

		case 3:
			registers[executeStage->destiny] = executeStage->operator01;
			dprintln("mov r%d, %d", executeStage->destiny, executeStage->operator01);
			break;
		default:
			printf("Instrução não implementada\n");
			executeStage->alive = false;
		}
		break;
	default:
		printf("Instrução não implementada\n");
		executeStage->alive = false;
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

	searchStage.pc = 1;
	executeStage.alive = true;
	int cycle = 1;
	while (executeStage.alive)
	{
		dprint("Ciclo de procesador: %d\n", cycle);
		switch (cycle)
		{
		case 1:
			search(&searchStage);
			dprint("Busca");
			break;
		case 2:
			decode(&searchStage, &decodeStage);
			search(&searchStage);
			dprint("Busca e decodifica");
			break;
		default:
			execute(&searchStage, &decodeStage, &executeStage);
			decode(&searchStage, &decodeStage);
			search(&searchStage);
			dprint("Busca, decodifica e executa");
			break;
		}
		dprint("\n-------------------\n");
		getchar();
		cycle++;
	}
	printf("Fim da execucao\n");
	print_registers();
	printf("\n");
	print_200_memory();
	printf("\n");
	return 0;
}