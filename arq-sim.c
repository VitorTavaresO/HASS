#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

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

uint16_t getBit(uint16_t num, int bit)
{
	return (num >> bit) & 0x01;
}

int main(int argc, char **argv)
{
	memory[0] = 0b0000000101110001;
	unsigned int test = (getBit(memory[0], 4) << 4) | (getBit(memory[0], 3) << 3) | (getBit(memory[0], 2) << 2) | (getBit(memory[0], 1) << 1) | getBit(memory[0], 0);
	printf("%d\n", test);

	return 0;
}