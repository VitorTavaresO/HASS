#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include "lib.h"
#include "lib.c"

// #define DEBUG

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

int execute_counter = 0;
int search_counter = 0;
float hit_counter = 0.0;
float miss_counter = 0.0;

typedef struct
{
	uint16_t pc;
	int8_t branch_taken;
	bool occupied;
	uint16_t target;
} bpt_entry;
bpt_entry bpt[BPT_SIZE];

enum STAGES
{
	SEARCH,
	DECODE_SEARCH,
	EXECUTE_DECODE_SEARCH,
	END
};
enum STAGES stage = SEARCH;

struct search_stage
{
	uint16_t pc;
	uint16_t instruction;
	uint16_t instruction_pc;
	uint16_t instruction_next_pc;
} search_stage;

struct decode_stage
{
	uint16_t instruction;
	uint16_t format;
	uint16_t opcode;
	uint16_t destiny;
	uint16_t operator01;
	uint16_t operator02;
	uint16_t instruction_pc;
	uint16_t instruction_next_pc;
	bool alive;
} decode_stage;

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

void init_bpt()
{
	for (int i = 0; i < BPT_SIZE; i++)
	{
		bpt[i].pc = 0;
		bpt[i].branch_taken = -1;
		bpt[i].occupied = 0;
	}
}

bpt_entry *predict_branch(uint16_t pc)
{
	bpt_entry *entry = &bpt[pc & BPT_MASK];
	if (entry->occupied == 1 && entry->pc == pc && entry->branch_taken == 1)
		return entry;
	else
		return NULL;
}

void update_bpt(uint16_t pc, uint8_t branch_taken, bool occupied, uint16_t target)
{
	bpt_entry *entry = &bpt[pc & BPT_MASK];
	entry->pc = pc;
	entry->branch_taken = branch_taken;
	entry->occupied = occupied;
	entry->target = target;
	dprintln("Foi inserido no BPT: pc: %d, branch_taken: %d, occupied: %d, target: %d", entry->pc, entry->branch_taken, entry->occupied, entry->target);
}

void search(struct search_stage *search_stage)
{
	const bpt_entry *entry = predict_branch(search_stage->pc);

	search_stage->instruction = memory[search_stage->pc];
	search_stage->instruction_pc = search_stage->pc;
	dprintln("Start pc: %d", search_stage->pc);

	if (entry)
		search_stage->pc = entry->target;
	else
		search_stage->pc++;

	search_stage->instruction_next_pc = search_stage->pc;
	dprintln("End pc: %d", search_stage->pc);
	dprintln("Stage: %d", stage);
	search_counter++;
}

void function_decode_r(uint16_t instruction, struct decode_stage *decode_stage)
{
	decode_stage->opcode = extract_bits(decode_stage->instruction, 9, 6);
	decode_stage->destiny = extract_bits(decode_stage->instruction, 6, 3);
	decode_stage->operator01 = extract_bits(decode_stage->instruction, 3, 3);
	decode_stage->operator02 = extract_bits(decode_stage->instruction, 0, 3);
}

void function_decode_i(uint16_t instruction, struct decode_stage *decode_stage)
{
	decode_stage->opcode = extract_bits(decode_stage->instruction, 13, 2);
	decode_stage->destiny = extract_bits(decode_stage->instruction, 10, 3);
	decode_stage->operator01 = extract_bits(decode_stage->instruction, 0, 10);
	decode_stage->operator02 = 0;
}

void decode(struct search_stage *search_stage, struct decode_stage *decode_stage)
{
	static void (*decodeFunctions[])(uint16_t, struct decode_stage *) = {
		function_decode_r, function_decode_i};

	decode_stage->instruction_pc = search_stage->instruction_pc;
	decode_stage->instruction_next_pc = search_stage->instruction_next_pc;
	decode_stage->instruction = search_stage->instruction;
	decode_stage->format = extract_bits(decode_stage->instruction, 15, 1);
	decodeFunctions[decode_stage->format](decode_stage->instruction, decode_stage);
}

void add(struct decode_stage *decode_stage)
{
	registers[decode_stage->destiny] = registers[decode_stage->operator01] + registers[decode_stage->operator02];
	dprintln("add r%d, r%d, r%d", decode_stage->destiny, decode_stage->operator01, decode_stage->operator02);
}

void sub(struct decode_stage *decode_stage)
{
	registers[decode_stage->destiny] = registers[decode_stage->operator01] - registers[decode_stage->operator02];
	dprintln("sub r%d, r%d, r%d", decode_stage->destiny, decode_stage->operator01, decode_stage->operator02);
}

void mul(struct decode_stage *decode_stage)
{
	registers[decode_stage->destiny] = registers[decode_stage->operator01] * registers[decode_stage->operator02];
	dprintln("mul r%d, r%d, r%d", decode_stage->destiny, decode_stage->operator01, decode_stage->operator02);
}

void divi(struct decode_stage *decode_stage)
{
	registers[decode_stage->destiny] = registers[decode_stage->operator01] / registers[decode_stage->operator02];
	dprintln("div r%d, r%d, r%d", decode_stage->destiny, decode_stage->operator01, decode_stage->operator02);
}

void cmp_equal(struct decode_stage *decode_stage)
{
	registers[decode_stage->destiny] = registers[decode_stage->operator01] == registers[decode_stage->operator02];
	dprintln("cmp_eq r%d, r%d, r%d", decode_stage->destiny, decode_stage->operator01, decode_stage->operator02);
}

void cmp_nequal(struct decode_stage *decode_stage)
{
	registers[decode_stage->destiny] = registers[decode_stage->operator01] != registers[decode_stage->operator02];
	dprintln("cmp_nq r%d, r%d, r%d", decode_stage->destiny, decode_stage->operator01, decode_stage->operator02);
}

void load(struct decode_stage *decode_stage)
{
	registers[decode_stage->destiny] = memory[registers[decode_stage->operator01]];
	dprintln("load r%d, (r%d)", decode_stage->destiny, decode_stage->operator01);
}

void store(struct decode_stage *decode_stage)
{
	memory[registers[decode_stage->operator01]] = registers[decode_stage->operator02];
	dprintln("store (r%d), r%d", decode_stage->operator01, decode_stage->operator02);
}

void syscall(struct decode_stage *decode_stage)
{
	switch (registers[decode_stage->operator01])
	{
	case 0:
		stage = END;
		decode_stage->alive = 0;
		printf("Fim da execucao\n");
		break;

	default:
		printf("Syscall nao implementada\n");
		break;
	}
}

void not_implemented_r(struct decode_stage *decode_stage)
{
	decode_stage->alive = 0;
	printf("Instrucao de Fomato R nao implementada\n");
}

void jump(struct search_stage *search_stage, struct decode_stage *decode_stage)
{
	if (decode_stage->instruction_next_pc == decode_stage->operator01)
	{
		dprint("Acertou desvio\n");
		hit_counter++;
	}
	else
	{
		dprint("Errou desvio\n");
		miss_counter++;
		stage = SEARCH;
		search_stage->pc = decode_stage->operator01;
	}
	update_bpt(decode_stage->instruction_pc, 1, 1, decode_stage->operator01);
	dprintln("jump %d\n", decode_stage->operator01);
}

void jump_cond(struct search_stage *search_stage, struct decode_stage *decode_stage)
{
	int branch_taken = registers[decode_stage->destiny] == 1;
	update_bpt(decode_stage->instruction_pc, branch_taken, 1, decode_stage->operator01);

	if ((branch_taken && decode_stage->instruction_next_pc == decode_stage->operator01)
	|| (!branch_taken && decode_stage->instruction_next_pc == decode_stage->instruction_pc + 1))
	{
		dprint("Acertou desvio\n");
		hit_counter++;
		switch (registers[decode_stage->destiny])
		{
		case 0:
			dprint("jump_cond nao atendida\n");
			break;
		case 1:
			dprint("jump_cond atendida\n");
			break;
		}
	}
	else
	{
		dprint("Errou desvio\n");
		miss_counter++;
		switch (registers[decode_stage->destiny])
		{
		case 0:
			dprint("jump_cond nao atendida\n");
			stage = SEARCH;
			search_stage->pc = decode_stage->instruction_pc + 1;
			break;
		case 1:
			dprint("jump_cond atendida\n");
			stage = SEARCH;
			search_stage->pc = decode_stage->operator01;
			break;
		}
	}
}

void mov(struct search_stage *search_stage, struct decode_stage *decode_stage)
{
	registers[decode_stage->destiny] = decode_stage->operator01;
	dprintln("mov r%d, %d", decode_stage->destiny, decode_stage->operator01);
}

void not_implemented_i(struct search_stage *search_stage, struct decode_stage *decode_stage)
{
	decode_stage->alive = 0;
	printf("Instrucao de Fomato I nao implementada\n");
}

void (*execute_functions_r[NUM_FUNCTIONS_R])(struct decode_stage *);

void (*execute_functions_i[NUM_FUNCTIONS_I])(struct search_stage *, struct decode_stage *);

void fill_tables()
{
	int i;

	for (i = 0; i < NUM_FUNCTIONS_R; i++)
	{
		execute_functions_r[i] = not_implemented_r;
	}

	execute_functions_r[0] = add;
	execute_functions_r[1] = sub;
	execute_functions_r[2] = mul;
	execute_functions_r[3] = divi;
	execute_functions_r[4] = cmp_equal;
	execute_functions_r[5] = cmp_nequal;
	execute_functions_r[15] = load;
	execute_functions_r[16] = store;
	execute_functions_r[63] = syscall;

	for (i = 0; i < NUM_FUNCTIONS_I; i++)
	{
		execute_functions_i[i] = not_implemented_i;
	}

	execute_functions_i[0] = jump;
	execute_functions_i[1] = jump_cond;
	execute_functions_i[3] = mov;
}

void execute_r(struct search_stage *search_stage, struct decode_stage *decode_stage)
{
	dprintln("formato R", decode_stage->format);
	dprintln("%d", decode_stage->opcode);
	(execute_functions_r[decode_stage->opcode](decode_stage));
}

void execute_i(struct search_stage *search_stage, struct decode_stage *decode_stage)
{
	dprintln("formato I", decode_stage->format);
	dprintln("%d", decode_stage->opcode);
	(execute_functions_i[decode_stage->opcode](search_stage, decode_stage));
}

void (*executeFormats[])(struct search_stage *, struct decode_stage *) = {
	execute_r,
	execute_i,
};

void execute(struct search_stage *search_stage, struct decode_stage *decode_stage)
{
	executeFormats[decode_stage->format](search_stage, decode_stage);
	execute_counter++;
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf("usage: %s [bin_name]\n", argv[0]);
		exit(1);
	}

	load_binary_to_memory(argv[1], memory, MEMORY_SIZE * 2);
	fill_tables();
	init_bpt();
	search_stage.pc = 1;
	decode_stage.alive = 1;
	int cycle = 1;
	while (decode_stage.alive)
	{
		dprint("Ciclo de procesador: %d\n", cycle);
		switch (stage)
		{
		case SEARCH:
			search(&search_stage);
			dprint("Busca");
			stage = DECODE_SEARCH;
			break;
		case DECODE_SEARCH:
			decode(&search_stage, &decode_stage);
			search(&search_stage);
			dprint("Busca e decodifica");
			stage = EXECUTE_DECODE_SEARCH;
			break;
		case EXECUTE_DECODE_SEARCH:
			execute(&search_stage, &decode_stage);
			if (stage != SEARCH)
			{
				decode(&search_stage, &decode_stage);
				search(&search_stage);
				dprint("Busca, decodifica e executa\n");
			}
			else
			{
				dprintln("Flush", 0);
			}

			break;
		}
		dprint("\n-------------------\n");

		// getchar();
		cycle++;
	}
	printf("Ciclos de processador: %d\n", cycle);
	printf("Ciclos de busca: %d\n", search_counter);
	printf("Ciclos de execucao: %d\n", execute_counter);
	printf("Previsoes acertadas: %1.f\n", hit_counter);
	printf("Previsoes erradas: %1.f\n", miss_counter);
	int hit_percentage = (hit_counter / (hit_counter + miss_counter)) * 100.0;
	printf("Taxa de acerto: %d%%\n", hit_percentage);
	print_registers();
	printf("\n");
	print_200_memory();
	printf("\n");
	return 0;
}