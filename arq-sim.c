#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>

#include "lib.h"

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

uint16_t (*operationsR[])(uint16_t, uint16_t) = {
	add,
	sub,
	mul,
	divi,
	cmp_equal,
	cmp_nequal};

uint16_t formatR(uint16_t registers[])
{
	registers[extract_bits(memory[0], 6, 3)] = operationsR[extract_bits(memory[0], 9, 6)](registers[extract_bits(memory[0], 3, 3)], registers[extract_bits(memory[0], 0, 3)]);
}

uint16_t mov(uint16_t a)
{
	return a;
}

uint16_t (*operationsL[])(uint16_t) = {
	mov};

uint16_t formatL(uint16_t registers[])
{
	registers[extract_bits(memory[0], 10, 3)] = operationsL[0](extract_bits(memory[0], 0, 10));
}

uint16_t (*formats[])(uint16_t registers[]) = {
	formatR,
	formatL};

uint16_t defineFormat(uint16_t registers[])
{
	formats[extract_bits(memory[0], 15, 1)](registers);
}

int main(int argc, char **argv)
{
	// if (argc != 2)
	//{
	//	printf("usage: %s <binfile>\n", argv[0]);
	//	exit(1);
	// }
	memory[0] = 0b1000110000111001;
	uint16_t registers[8];
	registers[1] = 10;
	registers[6] = 10;
	defineFormat(registers);
	printf("%d", registers[3]);
	return 0;
}

// uint16_t operator02 = extract_bits(memory[0], 0, 3);
// uint16_t operator01 = extract_bits(memory[0], 3, 3);
// uint16_t destiny = extract_bits(memory[0], 6, 3);
// uint16_t opcode = extract_bits(memory[0], 9, 6);
// uint16_t format = extract_bits(memory[0], 15, 1);