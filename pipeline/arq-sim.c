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
	const BPTEntry *entry = predictBranch(searchStage->pc);
	searchStage->instruction = memory[searchStage->pc];
	if (entry)
	{
		searchStage->pc = entry->target;
	}
	else
	{
		searchStage->pc++;
	}
	dprintln("pc: %d", searchStage->pc);
}

void decodeR(uint16_t instruction, struct decodeStage *decodeStage)
{
	decodeStage->opcode = extract_bits(decodeStage->instruction, 9, 6);
	decodeStage->destiny = extract_bits(decodeStage->instruction, 6, 3);
	decodeStage->operator01 = extract_bits(decodeStage->instruction, 3, 3);
	decodeStage->operator02 = extract_bits(decodeStage->instruction, 0, 3);
}

void decodeI(uint16_t instruction, struct decodeStage *decodeStage)
{
	decodeStage->opcode = extract_bits(decodeStage->instruction, 13, 2);
	decodeStage->destiny = extract_bits(decodeStage->instruction, 10, 3);
	decodeStage->operator01 = extract_bits(decodeStage->instruction, 0, 10);
	decodeStage->operator02 = 0;
}

void decode(struct searchStage *searchStage, struct decodeStage *decodeStage)
{
	decodeStage->instruction = searchStage->instruction;
	decodeStage->format = extract_bits(decodeStage->instruction, 15, 1);
	void (*decodeFunctions[])(uint16_t, struct decodeStage *) = {
		decodeR, decodeI};
	decodeFunctions[decodeStage->format](decodeStage->instruction, *&decodeStage);
}

void add(struct executeStage *executeStage)
{
	registers[executeStage->destiny] = registers[executeStage->operator01] + registers[executeStage->operator02];
	dprint("add r%d, r%d, r%d\n", executeStage->destiny, executeStage->operator01, executeStage->operator02);
}

void sub(struct executeStage *executeStage)
{
	registers[executeStage->destiny] = registers[executeStage->operator01] - registers[executeStage->operator02];
	dprintln("sub r%d, r%d, r%d", executeStage->destiny, executeStage->operator01, executeStage->operator02);
}

void mul(struct executeStage *executeStage)
{
	registers[executeStage->destiny] = registers[executeStage->operator01] * registers[executeStage->operator02];
	dprintln("mul r%d, r%d, r%d", executeStage->destiny, executeStage->operator01, executeStage->operator02);
}

void divi(struct executeStage *executeStage)
{
	registers[executeStage->destiny] = registers[executeStage->operator01] / registers[executeStage->operator02];
	dprintln("div r%d, r%d, r%d", executeStage->destiny, executeStage->operator01, executeStage->operator02);
}

void cmp_equal(struct executeStage *executeStage)
{
	registers[executeStage->destiny] = registers[executeStage->operator01] == registers[executeStage->operator02];
	dprintln("cmp_eq r%d, r%d, r%d", executeStage->destiny, executeStage->operator01, executeStage->operator02);
}

void cmp_nequal(struct executeStage *executeStage)
{
	registers[executeStage->destiny] = registers[executeStage->operator01] != registers[executeStage->operator02];
	dprintln("cmp_nq r%d, r%d, r%d", executeStage->destiny, executeStage->operator01, executeStage->operator02);
}

void load(struct executeStage *executeStage)
{
	registers[executeStage->destiny] = memory[registers[executeStage->operator01]];
	dprintln("load r%d, (r%d)", executeStage->destiny, executeStage->operator01);
}

void store(struct executeStage *executeStage)
{
	memory[registers[executeStage->operator01]] = registers[executeStage->operator02];
	dprintln("store (r%d), r%d", executeStage->operator01, executeStage->operator02);
}

void syscall(struct executeStage *executeStage)
{
	if (registers[executeStage->operator01] == 0)
	{
		stage = END;
		executeStage->alive = 0;
		printf("Fim da execucao\n");
	}
	else
	{
		stage = END;
		executeStage->alive = 0;
		printf("Syscall nao implementada\n");
	}
}

void not_implementedR(struct executeStage *executeStage)
{
	executeStage->alive = 0;
	printf("Instrucao de Fomato R nao implementada\n");
}

void jump(struct searchStage *searchStage, struct executeStage *executeStage)
{
	searchStage->pc = executeStage->operator01;
	dprintln("jump %d\n", executeStage->operator01);
}

void jump_cond(struct searchStage *searchStage, struct executeStage *executeStage)
{
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
}

void mov(struct searchStage *searchStage, struct executeStage *executeStage)
{
	registers[executeStage->destiny] = executeStage->operator01;
	dprintln("mov r%d, %d", executeStage->destiny, executeStage->operator01);
}

void not_implementedI(struct searchStage *searchStage, struct executeStage *executeStage)
{
	executeStage->alive = 0;
	printf("Instrucao de Fomato I nao implementada\n");
}

void (*executeFunctionsR[])(struct executeStage *) = {
	[0] = add,
	[1] = sub,
	[2] = mul,
	[3] = divi,
	[4] = cmp_equal,
	[5] = cmp_nequal,
	[15] = load,
	[16] = store,
	[63] = syscall,
};
void (*executeFunctionsI[])(struct searchStage *, struct executeStage *) = {
	[0] = jump,
	[1] = jump_cond,
	[3] = mov,
};

void executeR(struct searchStage *searchStage, struct executeStage *executeStage)
{
	dprintln("formato R", executeStage->format);
	(executeFunctionsR[executeStage->opcode] ? executeFunctionsR[executeStage->opcode] : not_implementedR)(executeStage);
}

void executeI(struct searchStage *searchStage, struct executeStage *executeStage)
{
	dprintln("formato I", executeStage->format);
	(executeFunctionsI[executeStage->opcode] ? executeFunctionsI[executeStage->opcode] : not_implementedI)(searchStage, executeStage);
}

void (*executeFormats[])(struct searchStage *, struct executeStage *) = {
	executeR,
	executeI,
};

void execute(struct searchStage *searchStage, struct decodeStage *decodeStage, struct executeStage *executeStage)
{
	executeStage->format = decodeStage->format;
	executeStage->opcode = decodeStage->opcode;
	executeStage->destiny = decodeStage->destiny;
	executeStage->operator01 = decodeStage->operator01;
	executeStage->operator02 = decodeStage->operator02;
	executeFormats[executeStage->format](searchStage, executeStage);
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
	executeStage.alive = 1;
	int cycle = 1;
	while (executeStage.alive)
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
			execute(&searchStage, &decodeStage, &executeStage);
			decode(&searchStage, &decodeStage);
			search(&searchStage);
			dprint("Busca, decodifica e executa");
			break;
		}
		dprint("\n-------------------\n");
		/*
		uint16_t pc = searchStage.pc;
		uint8_t branchTaken = predictBranch(pc);
		if(branchTaken){
			searchStage.pc = pc;
			stage = DECODE;
		}else {
			updateBpt(pc, 1);
		}
		*/

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