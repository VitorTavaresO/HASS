#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>

#define ADD 0b0000
#define SUB 0b0001
#define MUL 0b0010
#define DIV 0b0011

// #include "lib.h"

uint16_t memory[64 * 1024];

const char *get_reg_name_str(uint16_t reg)
{
	static const char *str[] = {
		"r0",
		"r1",
		"r2",
		"r3",
		"r4",
		"r5",
		"r6",
		"r7"};

	return str[reg];
}

uint16_t extract_bits(uint16_t v, uint8_t bstart, uint8_t blength)
{
	uint16_t mask;

	mask = (1 << blength) - 1;

	return ((v >> bstart) & mask);
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

uint16_t (*operations[])(uint16_t, uint16_t) = {
	add,
	sub,
	mul,
	divi};

int main(int argc, char **argv)
{
	// if (argc != 2)
	//{
	//	printf("usage: %s <binfile>\n", argv[0]);
	//	exit(1);
	// }
	memory[0] = 0b000000101110001;
	uint16_t registers[8];
	registers[1] = 10;
	registers[6] = 20;
	registers[extract_bits(memory[0], 6, 3)] = operations[extract_bits(memory[0], 9, 6)](registers[extract_bits(memory[0], 3, 3)], registers[extract_bits(memory[0], 0, 3)]);
	printf("%d", registers[extract_bits(memory[0], 6, 3)]);
	// uint16_t operator02 = extract_bits(memory[0], 0, 3);
	// uint16_t operator01 = extract_bits(memory[0], 3, 3);
	// uint16_t destiny = extract_bits(memory[0], 6, 3);
	// uint16_t opcode = extract_bits(memory[0], 9, 6);
	// uint16_t format = extract_bits(memory[0], 15, 1);

	return 0;
}