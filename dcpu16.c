#include <stdio.h>
#include <string.h>
#include "dcpu16.h"


DCPU16_WORD reg_a, reg_b, reg_c, reg_x, reg_y, reg_z, reg_i, reg_j, reg_pc, reg_sp, reg_o;
DCPU16_WORD ram[DCPU16_RAM_SIZE];

int dcpu16_init(char * ram_file)
{
	// Set registers
	reg_a = 0;
	reg_b = 0;
	reg_c = 0;
	reg_x = 0;
	reg_y = 0;
	reg_z = 0;
	reg_i = 0;
	reg_j = 0;

	reg_pc = 0;
	reg_sp = 0xFFFF;
	reg_o = 0;

	// Load the RAM
	memset(ram, 0 , sizeof(ram));

	FILE * rf = fopen(ram_file, "r");
	
	if(!rf) {
		printf("couldn't load ram from file\n");
		return 0;
	}

	int n = fread((char *) ram, sizeof(ram) * sizeof(DCPU16_WORD), 1, rf);
	fclose(rf);

	return 1;
}

void dcpu16_run() 
{
	while(pc < sizeof(ram))
		step();
}

void dcpu16_debug_execute(DCPU16_WORD w)
{
	printf("executing %X at location 0\n", w);

	ram[0] = w;
	reg_pc = 0;

	printf("cycles: %u\n", dcpu16_step());
}

int dcpu16_get_location(char loc, DCPU16_WORD ** retval)
{
	switch(loc) {
		case DCPU16_AB_VALUE_REG_A:
			*retval = (DCPU16_WORD *) &reg_a;
			return 0;
		case DCPU16_AB_VALUE_REG_B:
			*retval = (DCPU16_WORD *) &reg_b;
			return 0;
		case DCPU16_AB_VALUE_REG_C:
			*retval = (DCPU16_WORD *) &reg_c;
			return 0;
		case DCPU16_AB_VALUE_REG_X:
			*retval = (DCPU16_WORD *) &reg_x;
			return 0;
		case DCPU16_AB_VALUE_REG_Y:
			*retval = (DCPU16_WORD *) &reg_y;
			return 0;
		case DCPU16_AB_VALUE_REG_Z:
			*retval = (DCPU16_WORD *) &reg_z;
			return 0;
		case DCPU16_AB_VALUE_REG_I:
			*retval = (DCPU16_WORD *) &reg_i;
			return 0;
		case DCPU16_AB_VALUE_REG_J:
			*retval = (DCPU16_WORD *) &reg_j;
			return 0;
		case DCPU16_AB_VALUE_PTR_REG_A:
			*retval = (DCPU16_WORD *) &ram[reg_a];
			return 0;
		case DCPU16_AB_VALUE_PTR_REG_B:
			*retval = (DCPU16_WORD *) &ram[reg_b];
			return 0;
		case DCPU16_AB_VALUE_PTR_REG_C:
			*retval = (DCPU16_WORD *) &ram[reg_c];
			return 0;
		case DCPU16_AB_VALUE_PTR_REG_X:
			*retval = (DCPU16_WORD *) &ram[reg_x];
			return 0;
		case DCPU16_AB_VALUE_PTR_REG_Y:
			*retval = (DCPU16_WORD *) &ram[reg_y];
			return 0;
		case DCPU16_AB_VALUE_PTR_REG_Z:
			*retval = (DCPU16_WORD *) &ram[reg_z];
			return 0;
		case DCPU16_AB_VALUE_PTR_REG_I:
			*retval = (DCPU16_WORD *) &ram[reg_i];
			return 0;
		case DCPU16_AB_VALUE_PTR_REG_J:
			*retval = (DCPU16_WORD *) &ram[reg_j];
			return 0;
		case DCPU16_AB_VALUE_PTR_REG_A_PLUS_WORD:
			*retval = (DCPU16_WORD *) &ram[reg_a + ram[reg_pc + 1]];
			reg_pc++;
			return 1;
		case DCPU16_AB_VALUE_PTR_REG_B_PLUS_WORD:
			*retval = (DCPU16_WORD *) &ram[reg_b + ram[reg_pc + 1]];
			reg_pc++;
			return 1;
		case DCPU16_AB_VALUE_PTR_REG_C_PLUS_WORD:
			*retval = (DCPU16_WORD *) &ram[reg_c + ram[reg_pc + 1]];
			reg_pc++;
			return 1;
		case DCPU16_AB_VALUE_PTR_REG_X_PLUS_WORD:
			*retval = (DCPU16_WORD *) &ram[reg_x + ram[reg_pc + 1]];
			reg_pc++;
			return 1;
		case DCPU16_AB_VALUE_PTR_REG_Y_PLUS_WORD:
			*retval = (DCPU16_WORD *) &ram[reg_y + ram[reg_pc + 1]];
			reg_pc++;
			return 1;
		case DCPU16_AB_VALUE_PTR_REG_Z_PLUS_WORD:
			*retval = (DCPU16_WORD *) &ram[reg_z + ram[reg_pc + 1]];
			reg_pc++;
			return 1;
		case DCPU16_AB_VALUE_PTR_REG_I_PLUS_WORD:
			*retval = (DCPU16_WORD *) &ram[reg_i + ram[reg_pc + 1]];
			reg_pc++;
			return 1;
		case DCPU16_AB_VALUE_PTR_REG_J_PLUS_WORD:
			*retval = (DCPU16_WORD *) &ram[reg_j + ram[reg_pc + 1]];
			reg_pc++;
			return 1;
		case DCPU16_AB_VALUE_POP:
			*retval = (DCPU16_WORD *) &ram[reg_sp++];
			return 0;
		case DCPU16_AB_VALUE_PEEK:
			*retval = (DCPU16_WORD *) &ram[reg_sp];
			return 0;
		case DCPU16_AB_VALUE_PUSH:
			*retval = (DCPU16_WORD *) &ram[++reg_sp];
			return 0;
		case DCPU16_AB_VALUE_REG_SP:
			*retval = (DCPU16_WORD *) &reg_sp;
			return 0;
		case DCPU16_AB_VALUE_REG_PC:
			*retval = (DCPU16_WORD *) &reg_pc;
			return 0;
		case DCPU16_AB_VALUE_REG_O:
			*retval = (DCPU16_WORD *) &reg_o;
			return 0;
		case DCPU16_AB_VALUE_PTR_WORD:
			*retval = (DCPU16_WORD *) &ram[ram[reg_pc + 1]];
			reg_pc++;
			return 1;
		case DCPU16_AB_VALUE_WORD:
			*retval = (DCPU16_WORD *) &ram[reg_pc + 1];
			reg_pc++;
			return 1;
	};

	*retval = 0;
	return 0;
}

char dcpu16_is_literal(char loc)
{
	if(loc == DCPU16_AB_VALUE_PTR_WORD || loc == DCPU16_AB_VALUE_WORD)
		return 1;

	return 0;
}

int dcpu16_step() 
{
	DCPU16_WORD w = ram[reg_pc];
	char opcode = w & 0xF;

	if(opcode != DCPU16_OPCODE_NON_BASIC) {

		char a = (w >> 4) & 0x3F;
		char b = (w >> 10) & 0x3F;

		DCPU16_WORD * src;
		DCPU16_WORD * dst;
		int cycles = dcpu16_get_location(b, &src);
		cycles += dcpu16_get_location(a, &dst);

		char dst_is_literal = dcpu16_is_literal(a);

		// Increase PC (opcode)
		reg_pc++;

		switch(opcode) {
		case DCPU16_OPCODE_SET:
			cycles += 1;

			if(!dst_is_literal) {
				*dst = *src;
			}

			return cycles;
		case DCPU16_OPCODE_ADD:
			cycles += 2;

			if(!dst_is_literal) {
				if((int) *dst + (int) *src > 65535) {
					reg_o = 1;
				} else {
					reg_o = 0;
				}

				*dst = *dst + *src;
			}

			return cycles;
		case DCPU16_OPCODE_SUB:
			cycles += 2;

			if(!dst_is_literal) {
				if((int) *dst - (int) *src < 0) {
					reg_o = 0xFFFF;
				} else {
					reg_o = 0;
				}

				*dst = *dst - *src;
			}

			return cycles;
		case DCPU16_OPCODE_MUL:
			cycles += 2;

			if(!dst_is_literal) {
				reg_o = ((*dst * *src) >> 16) & 0xFFFF;
				*dst = *dst * *src;
			}

			return cycles;
		case DCPU16_OPCODE_DIV:
			cycles += 3;

			if(!dst_is_literal) {
				if(*src == 0) {
					reg_a = 0;
					reg_o = 0;
				} else {
					reg_o = ((*dst << 16) / b) & 0xFFFF;
					*dst = *dst / *src;
				}
			}

			return cycles;
		case DCPU16_OPCODE_MOD:
			cycles += 3;

			if(!dst_is_literal) {
				if(*src == 0) {
					reg_a = 0;
				} else {
					*dst = *dst % *src;
				}
			}

			return cycles;
		case DCPU16_OPCODE_SHL:
			cycles += 2;

			if(!dst_is_literal) {
				reg_o = ((*dst << *src) >> 16) & 0xFFFF;
				*dst = *dst << *src;
			}

			return cycles;
		case DCPU16_OPCODE_SHR:
			cycles += 2;

			if(!dst_is_literal) {
				reg_o = ((*dst << 16) >> *src) & 0xFFFF;
				*dst = *dst >> *src;
			}

			return cycles;
		case DCPU16_OPCODE_AND:
			cycles += 1;

			if(!dst_is_literal) {
				*dst = *dst & *src;
			}

			return cycles;
		case DCPU16_OPCODE_BOR:
			cycles += 1;

			if(!dst_is_literal) {
				*dst = *dst | *src;
			}

			return cycles;
		case DCPU16_OPCODE_XOR:
			cycles += 1;

			if(!dst_is_literal) {
				*dst = *dst ^ *src;
			}

			return cycles;
		case DCPU16_OPCODE_IFE:
			cycles += 2;
			
			if(*dst != *src)
			{
				reg_pc++;
				cycles++;
			}

			return cycles;
		case DCPU16_OPCODE_IFN:
			cycles += 2;

			if(*dst == *src)
			{
				reg_pc++;
				cycles++;
			}

			return cycles;
		case DCPU16_OPCODE_IFG:
			cycles += 2;

			if(*dst < *src)
			{
				reg_pc++;
				cycles++;
			}

			return cycles;
		case DCPU16_OPCODE_IFB:
			cycles += 2;	

			if(*dst & *src == 0)
			{
				reg_pc++;
				cycles++;
			}

			return cycles;
		};
	} else {
		// Non-basic
	}
}

void main(int argc, char * argv[]) 
{
	char * ram_file = argv[1];

	dcpu16_init(ram_file);
	dcpu16_run();
}
