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

#define MEMORY_SIZE (64 * 1024)
#define REGISTERS 8
#define BPT_SIZE_BITS 10
#define BPT_SIZE (1 << BPT_SIZE_BITS)
#define BPT_MASK (BPT_SIZE - 1)
#define NUM_FUNCTIONS_R 64
#define NUM_FUNCTIONS_I 4

uint16_t memory[MEMORY_SIZE];
uint16_t registers[REGISTERS];

typedef struct
{
	uint16_t pc;
	int8_t branchTaken;
	bool occupied;
	uint16_t target;
} BPTEntry;
BPTEntry bpt[BPT_SIZE];

void initBpt()
{
	for (int i = 0; i < BPT_SIZE; i++)
	{
		bpt[i].pc = 0;
		bpt[i].branchTaken = -1;
		bpt[i].occupied = 0;
	}
}

BPTEntry *predictBranch(uint16_t pc)
{
	BPTEntry *entry = &bpt[pc & BPT_MASK];
	if (entry->occupied == 1 && entry->pc == pc && entry->branchTaken == 1)
	{
		return entry;
	}
	else
	{
		return NULL;
	}
}

void updateBpt(uint16_t pc, uint8_t branchTaken, bool occupied, uint16_t target)
{
	BPTEntry *entry = &bpt[pc & BPT_MASK];
	entry->pc = pc;
	entry->branchTaken = branchTaken;
	entry->occupied = occupied;
	entry->target = target;
	dprintln("Foi inserido no BPT: pc: %d, branchTaken: %d, occupied: %d, target: %d", entry->pc, entry->branchTaken, entry->occupied, entry->target);
}

enum STAGES
{
	SEARCH,
	DECODE_SEARCH,
	EXECUTE_DECODE_SEARCH,
	END
};
enum STAGES stage = SEARCH;

struct searchStage
{
	uint16_t pc;
	uint16_t instruction;
	uint16_t instructionPc;
	uint16_t instructionNextPc;
} searchStage;

struct decodeStage
{
	uint16_t instruction;
	uint16_t format;
	uint16_t opcode;
	uint16_t destiny;
	uint16_t operator01;
	uint16_t operator02;
	uint16_t instructionPc;
	uint16_t instructionNextPc;
	bool alive;
} decodeStage;

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
	const BPTEntry *entry = predictBranch(searchStage->pc);
	searchStage->instruction = memory[searchStage->pc];
	searchStage->instructionPc = searchStage->pc;
	dprintln("Start pc: %d", searchStage->pc);
	if (entry)
	{
		searchStage->pc = entry->target;
	}
	else
	{
		searchStage->pc++;
	}
	searchStage->instructionNextPc = searchStage->pc;
	dprintln("End pc: %d", searchStage->pc);
	dprintln("Stage: %d", stage);
}

void function_decode_R(uint16_t instruction, struct decodeStage *decodeStage)
{
	decodeStage->opcode = extract_bits(decodeStage->instruction, 9, 6);
	decodeStage->destiny = extract_bits(decodeStage->instruction, 6, 3);
	decodeStage->operator01 = extract_bits(decodeStage->instruction, 3, 3);
	decodeStage->operator02 = extract_bits(decodeStage->instruction, 0, 3);
}

void function_decode_I(uint16_t instruction, struct decodeStage *decodeStage)
{
	decodeStage->opcode = extract_bits(decodeStage->instruction, 13, 2);
	decodeStage->destiny = extract_bits(decodeStage->instruction, 10, 3);
	decodeStage->operator01 = extract_bits(decodeStage->instruction, 0, 10);
	decodeStage->operator02 = 0;
}

void decode(struct searchStage *searchStage, struct decodeStage *decodeStage)
{
	decodeStage->instructionPc = searchStage->instructionPc;
	decodeStage->instructionNextPc = searchStage->instructionNextPc;
	decodeStage->instruction = searchStage->instruction;
	decodeStage->format = extract_bits(decodeStage->instruction, 15, 1);
	void (*decodeFunctions[])(uint16_t, struct decodeStage *) = {
		function_decode_R, function_decode_I};
	decodeFunctions[decodeStage->format](decodeStage->instruction, *&decodeStage);
}

void add(struct decodeStage *decodeStage)
{
	registers[decodeStage->destiny] = registers[decodeStage->operator01] + registers[decodeStage->operator02];
	dprint("add r%d, r%d, r%d\n", decodeStage->destiny, decodeStage->operator01, decodeStage->operator02);
}

void sub(struct decodeStage *decodeStage)
{
	registers[decodeStage->destiny] = registers[decodeStage->operator01] - registers[decodeStage->operator02];
	dprintln("sub r%d, r%d, r%d", decodeStage->destiny, decodeStage->operator01, decodeStage->operator02);
}

void mul(struct decodeStage *decodeStage)
{
	registers[decodeStage->destiny] = registers[decodeStage->operator01] * registers[decodeStage->operator02];
	dprintln("mul r%d, r%d, r%d", decodeStage->destiny, decodeStage->operator01, decodeStage->operator02);
}

void divi(struct decodeStage *decodeStage)
{
	registers[decodeStage->destiny] = registers[decodeStage->operator01] / registers[decodeStage->operator02];
	dprintln("div r%d, r%d, r%d", decodeStage->destiny, decodeStage->operator01, decodeStage->operator02);
}

void cmp_equal(struct decodeStage *decodeStage)
{
	registers[decodeStage->destiny] = registers[decodeStage->operator01] == registers[decodeStage->operator02];
	dprintln("cmp_eq r%d, r%d, r%d", decodeStage->destiny, decodeStage->operator01, decodeStage->operator02);
}

void cmp_nequal(struct decodeStage *decodeStage)
{
	registers[decodeStage->destiny] = registers[decodeStage->operator01] != registers[decodeStage->operator02];
	dprintln("cmp_nq r%d, r%d, r%d", decodeStage->destiny, decodeStage->operator01, decodeStage->operator02);
}

void load(struct decodeStage *decodeStage)
{
	registers[decodeStage->destiny] = memory[registers[decodeStage->operator01]];
	dprintln("load r%d, (r%d)", decodeStage->destiny, decodeStage->operator01);
}

void store(struct decodeStage *decodeStage)
{
	memory[registers[decodeStage->operator01]] = registers[decodeStage->operator02];
	dprintln("store (r%d), r%d", decodeStage->operator01, decodeStage->operator02);
}

void syscall(struct decodeStage *decodeStage)
{
	if (registers[decodeStage->operator01] == 0)
	{
		stage = END;
		decodeStage->alive = 0;
		printf("Fim da execucao\n");
	}
	else
	{
		printf("Syscall nao implementada\n");
	}
}

void not_implementedR(struct decodeStage *decodeStage)
{
	decodeStage->alive = 0;
	printf("Instrucao de Fomato R nao implementada\n");
}

void jump(struct searchStage *searchStage, struct decodeStage *decodeStage)
{
	if (decodeStage->instructionNextPc == decodeStage->operator01)
	{
		dprintln("Acertou desvio", 0);
	}
	else
	{
		dprintln("Errou desvio", 0);
		stage = SEARCH;
		searchStage->pc = decodeStage->operator01;
	}
	updateBpt(decodeStage->instructionPc, 1, 1, decodeStage->operator01);
	dprintln("jump %d\n", decodeStage->operator01);
}

void jump_cond(struct searchStage *searchStage, struct decodeStage *decodeStage)
{
	int branchTaken = registers[decodeStage->destiny] == 1;
	updateBpt(decodeStage->instructionPc, branchTaken, 1, decodeStage->operator01);

	if ((branchTaken && decodeStage->instructionNextPc == decodeStage->operator01) || (!branchTaken && decodeStage->instructionNextPc == decodeStage->instructionPc + 1))
	{
		dprintln("Acertou desvio", 0);
		switch (registers[decodeStage->destiny])
		{
		case 0:
			dprintln("jump_cond nao atendida", 0);
			break;
		case 1:
			dprintln("jump_cond atendida", 1);
			break;
		}
	}
	else
	{
		dprintln("Errou desvio", 0);
		switch (registers[decodeStage->destiny])
		{
		case 0:
			dprintln("jump_cond nao atendida", 0);
			stage = SEARCH;
			searchStage->pc = decodeStage->instructionPc + 1;
			break;
		case 1:
			dprintln("jump_cond atendida", 1);
			stage = SEARCH;
			searchStage->pc = decodeStage->operator01;
			break;
		}
	}
}

void mov(struct searchStage *searchStage, struct decodeStage *decodeStage)
{
	registers[decodeStage->destiny] = decodeStage->operator01;
	dprintln("mov r%d, %d", decodeStage->destiny, decodeStage->operator01);
}

void not_implementedI(struct searchStage *searchStage, struct decodeStage *decodeStage)
{
	decodeStage->alive = 0;
	printf("Instrucao de Fomato I nao implementada\n");
}

void (*executeFunctionsR[NUM_FUNCTIONS_R])(struct decodeStage *);

void (*executeFunctionsI[NUM_FUNCTIONS_I])(struct searchStage *, struct decodeStage *);

void fillTables()
{
	int i;

	for (i = 0; i < NUM_FUNCTIONS_R; i++)
	{
		executeFunctionsR[i] = not_implementedR;
	}

	executeFunctionsR[0] = add;
	executeFunctionsR[1] = sub;
	executeFunctionsR[2] = mul;
	executeFunctionsR[3] = divi;
	executeFunctionsR[4] = cmp_equal;
	executeFunctionsR[5] = cmp_nequal;
	executeFunctionsR[15] = load;
	executeFunctionsR[16] = store;
	executeFunctionsR[63] = syscall;

	for (i = 0; i < NUM_FUNCTIONS_I; i++)
	{
		executeFunctionsI[i] = not_implementedI;
	}

	executeFunctionsI[0] = jump;
	executeFunctionsI[1] = jump_cond;
	executeFunctionsI[3] = mov;
}

void executeR(struct searchStage *searchStage, struct decodeStage *decodeStage)
{
	dprintln("formato R", decodeStage->format);
	dprintln("%d", decodeStage->opcode);
	(executeFunctionsR[decodeStage->opcode](decodeStage));
}

void executeI(struct searchStage *searchStage, struct decodeStage *decodeStage)
{
	dprintln("formato I", decodeStage->format);
	dprintln("%d", decodeStage->opcode);
	(executeFunctionsI[decodeStage->opcode](searchStage, decodeStage));
}

void (*executeFormats[])(struct searchStage *, struct decodeStage *) = {
	executeR,
	executeI,
};

void execute(struct searchStage *searchStage, struct decodeStage *decodeStage)
{
	executeFormats[decodeStage->format](searchStage, decodeStage);
}
int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf("usage: %s [bin_name]\n", argv[0]);
		exit(1);
	}

	load_binary_to_memory(argv[1], memory, MEMORY_SIZE * 2);
	fillTables();
	initBpt();
	searchStage.pc = 1;
	decodeStage.alive = 1;
	int cycle = 1;
	while (decodeStage.alive)
	{
		dprint("Ciclo de procesador: %d\n", cycle);
		switch (stage)
		{
		case SEARCH:
			search(&searchStage);
			dprint("Busca");
			stage = DECODE_SEARCH;
			break;
		case DECODE_SEARCH:
			decode(&searchStage, &decodeStage);
			search(&searchStage);
			dprint("Busca e decodifica");
			stage = EXECUTE_DECODE_SEARCH;
			break;
		case EXECUTE_DECODE_SEARCH:
			execute(&searchStage, &decodeStage);
			if (stage != SEARCH)
			{
				decode(&searchStage, &decodeStage);
				search(&searchStage);
				dprint("Busca, decodifica e executa\n");
			}
			else
			{
				dprintln("Flush", 0);
			}
			print_registers();

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