#include <stdio.h>
#include <string.h>
#include "dcpu16.h"


DCPU16_WORD reg_a, reg_b, reg_c, reg_x, reg_y, reg_z, reg_i, reg_j, reg_pc, reg_sp, reg_o;
DCPU16_WORD ram[DCPU16_RAM_SIZE];

DCPU16_WORD a_literal_value, b_literal_value;

void dcpu16_print_registers()
{
	printf("--------------------------------------------------------------\n");
	printf("a:%x b:%x c:%x x:%x y:%x z:%x i:%x j:%x pc:%x sp:%x o:%x\n", 
		reg_a, reg_b, reg_c, reg_x, reg_y, reg_z, reg_i, reg_j, reg_pc, reg_sp, reg_o);
	printf("--------------------------------------------------------------\n");
}

void dcpu16_dump_ram(unsigned int start, unsigned int end)
{
	if(end >= DCPU16_RAM_SIZE)
		end = DCPU16_RAM_SIZE - 1;

	for(; start <= end; start++)
	{
		DCPU16_WORD w = ram[start];
		printf("RAM %.4x:\t%.4x\n", start, w);
	}
}

int dcpu16_get_pointer(char v, char a, DCPU16_WORD ** retval)
{
	switch(v) {
		case DCPU16_AB_VALUE_REG_A:
			*retval = &reg_a;
			return 0;
		case DCPU16_AB_VALUE_REG_B:
			*retval = &reg_b;
			return 0;
		case DCPU16_AB_VALUE_REG_C:
			*retval = &reg_c;
			return 0;
		case DCPU16_AB_VALUE_REG_X:
			*retval = &reg_x;
			return 0;
		case DCPU16_AB_VALUE_REG_Y:
			*retval = &reg_y;
			return 0;
		case DCPU16_AB_VALUE_REG_Z:
			*retval = &reg_z;
			return 0;
		case DCPU16_AB_VALUE_REG_I:
			*retval = &reg_i;
			return 0;
		case DCPU16_AB_VALUE_REG_J:
			*retval = &reg_j;
			return 0;
		case DCPU16_AB_VALUE_PTR_REG_A:
			*retval = &ram[reg_a];
			return 0;
		case DCPU16_AB_VALUE_PTR_REG_B:
			*retval = &ram[reg_b];
			return 0;
		case DCPU16_AB_VALUE_PTR_REG_C:
			*retval = &ram[reg_c];
			return 0;
		case DCPU16_AB_VALUE_PTR_REG_X:
			*retval = &ram[reg_x];
			return 0;
		case DCPU16_AB_VALUE_PTR_REG_Y:
			*retval = &ram[reg_y];
			return 0;
		case DCPU16_AB_VALUE_PTR_REG_Z:
			*retval = &ram[reg_z];
			return 0;
		case DCPU16_AB_VALUE_PTR_REG_I:
			*retval = &ram[reg_i];
			return 0;
		case DCPU16_AB_VALUE_PTR_REG_J:
			*retval = &ram[reg_j];
			return 0;
		case DCPU16_AB_VALUE_PTR_REG_A_PLUS_WORD:
			*retval = &ram[reg_a + ram[reg_pc + 1]];
			if(*retval >= &ram[DCPU16_RAM_SIZE]) {
				// Fix ram out of bounds (start over at zero)
				*retval = (DCPU16_WORD *)((*retval - ram) % DCPU16_RAM_SIZE);
			}

			reg_pc++;
			return 1;
		case DCPU16_AB_VALUE_PTR_REG_B_PLUS_WORD:
			*retval = &ram[reg_b + ram[reg_pc + 1]];
			if(*retval >= &ram[DCPU16_RAM_SIZE]) {
				// Fix ram out of bounds (start over at zero)
				*retval = (DCPU16_WORD *)((*retval - ram) % DCPU16_RAM_SIZE);
			}

			reg_pc++;
			return 1;
		case DCPU16_AB_VALUE_PTR_REG_C_PLUS_WORD:
			*retval = &ram[reg_c + ram[reg_pc + 1]];
			if(*retval >= &ram[DCPU16_RAM_SIZE]) {
				// Fix ram out of bounds (start over at zero)
				*retval = (DCPU16_WORD *)((*retval - ram) % DCPU16_RAM_SIZE);
			}

			reg_pc++;
			return 1;
		case DCPU16_AB_VALUE_PTR_REG_X_PLUS_WORD:
			*retval = &ram[reg_x + ram[reg_pc + 1]];
			if(*retval >= &ram[DCPU16_RAM_SIZE]) {
				// Fix ram out of bounds (start over at zero)
				*retval = (DCPU16_WORD *)((*retval - ram) % DCPU16_RAM_SIZE);
			}

			reg_pc++;
			return 1;
		case DCPU16_AB_VALUE_PTR_REG_Y_PLUS_WORD:
			*retval = &ram[reg_y + ram[reg_pc + 1]];
			if(*retval >= &ram[DCPU16_RAM_SIZE]) {
				// Fix ram out of bounds (start over at zero)
				*retval = (DCPU16_WORD *)((*retval - ram) % DCPU16_RAM_SIZE);
			}

			reg_pc++;
			return 1;
		case DCPU16_AB_VALUE_PTR_REG_Z_PLUS_WORD:
			*retval = &ram[reg_z + ram[reg_pc + 1]];
			if(*retval >= &ram[DCPU16_RAM_SIZE]) {
				// Fix ram out of bounds (start over at zero)
				*retval = (DCPU16_WORD *)((*retval - ram) % DCPU16_RAM_SIZE);
			}

			reg_pc++;
			return 1;
		case DCPU16_AB_VALUE_PTR_REG_I_PLUS_WORD:
			*retval = &ram[reg_i + ram[reg_pc + 1]];
			if(*retval >= &ram[DCPU16_RAM_SIZE]) {
				// Fix ram out of bounds (start over at zero)
				*retval = (DCPU16_WORD *)((*retval - ram) % DCPU16_RAM_SIZE);
			}

			reg_pc++;
			return 1;
		case DCPU16_AB_VALUE_PTR_REG_J_PLUS_WORD:
			*retval = &ram[reg_j + ram[reg_pc + 1]];
			if(*retval >= &ram[DCPU16_RAM_SIZE]) {
				// Fix ram out of bounds (start over at zero)
				*retval = (DCPU16_WORD *)((*retval - ram) % DCPU16_RAM_SIZE);
			}

			reg_pc++;
			return 1;
		case DCPU16_AB_VALUE_POP:
			*retval = &ram[reg_sp++];

			if(reg_sp >= DCPU16_RAM_SIZE)
			{
				// Fix out of bounds
				reg_sp = reg_sp % DCPU16_RAM_SIZE;
			}

			return 0;
		case DCPU16_AB_VALUE_PEEK:
			*retval = &ram[reg_sp];
			return 0;
		case DCPU16_AB_VALUE_PUSH:
			if(reg_sp <= 0)
			{
				// Fix out of bounds
				reg_sp = DCPU16_RAM_SIZE - 1 + reg_sp;
			} else {
				reg_sp--;
			}

			*retval = &ram[reg_sp];

			return 0;
		case DCPU16_AB_VALUE_REG_SP:
			*retval = &reg_sp;
			return 0;
		case DCPU16_AB_VALUE_REG_PC:
			*retval = &reg_pc;
			return 0;
		case DCPU16_AB_VALUE_REG_O:
			*retval = &reg_o;
			return 0;
		case DCPU16_AB_VALUE_PTR_WORD:
			*retval = &ram[ram[reg_pc]];
			reg_pc++;
			return 1;
		case DCPU16_AB_VALUE_WORD:
			*retval = &ram[reg_pc];
			reg_pc++;
			return 1;
	};

	if(v >= 0x20 && v <= 0x3F) {
		if(a == 1) {
			a_literal_value = v - 0x20;
			*retval = &a_literal_value;
		} else if(a == 0) {
			b_literal_value = v - 0x20;
			*retval = &b_literal_value;
		}

		return 0;
	}


	*retval = 0;
	return 0;
}

char dcpu16_is_literal(char v)
{
	if(v == DCPU16_AB_VALUE_WORD || (v >= 0x20 && v <= 0x3F))
		return 1;

	return 0;
}

int dcpu16_step() 
{
	DCPU16_WORD w = ram[reg_pc];
	char opcode = w & 0xF;
	int cycles = 0;

	// Increase PC (opcode)
	reg_pc++;

	if(opcode != DCPU16_OPCODE_NON_BASIC) {

		char a = (w >> 4) & 0x3F;
		char b = (w >> 10) & 0x3F;

		DCPU16_WORD * b_word;
		DCPU16_WORD * a_word;
		cycles += dcpu16_get_pointer(a, 1, &a_word);
		cycles += dcpu16_get_pointer(b, 0, &b_word);

		char a_word_is_literal = dcpu16_is_literal(a);

		switch(opcode) {
		case DCPU16_OPCODE_SET:
			cycles += 1;

			if(!a_word_is_literal) {
				*a_word = *b_word;
			}

			return cycles;
		case DCPU16_OPCODE_ADD:
			cycles += 2;

			if(!a_word_is_literal) {
				if((int) *a_word + (int) *b_word > 65535) {
					reg_o = 1;
				} else {
					reg_o = 0;
				}

				*a_word = *a_word + *b_word;
			}

			return cycles;
		case DCPU16_OPCODE_SUB:
			cycles += 2;

			if(!a_word_is_literal) {
				if((int) *a_word - (int) *b_word < 0) {
					reg_o = 0xFFFF;
				} else {
					reg_o = 0;
				}

				*a_word = *a_word - *b_word;
			}

			return cycles;
		case DCPU16_OPCODE_MUL:
			cycles += 2;

			if(!a_word_is_literal) {
				reg_o = ((*a_word * *b_word) >> 16) & 0xFFFF;
				*a_word = *a_word * *b_word;
			}

			return cycles;
		case DCPU16_OPCODE_DIV:
			cycles += 3;

			if(!a_word_is_literal) {
				if(*b_word == 0) {
					reg_a = 0;
					reg_o = 0;
				} else {
					reg_o = ((*a_word << 16) / b) & 0xFFFF;
					*a_word = *a_word / *b_word;
				}
			}

			return cycles;
		case DCPU16_OPCODE_MOD:
			cycles += 3;

			if(!a_word_is_literal) {
				if(*b_word == 0) {
					reg_a = 0;
				} else {
					*a_word = *a_word % *b_word;
				}
			}

			return cycles;
		case DCPU16_OPCODE_SHL:
			cycles += 2;

			if(!a_word_is_literal) {
				reg_o = ((*a_word << *b_word) >> 16) & 0xFFFF;
				*a_word = *a_word << *b_word;
			}

			return cycles;
		case DCPU16_OPCODE_SHR:
			cycles += 2;

			if(!a_word_is_literal) {
				reg_o = ((*a_word << 16) >> *b_word) & 0xFFFF;
				*a_word = *a_word >> *b_word;
			}

			return cycles;
		case DCPU16_OPCODE_AND:
			cycles += 1;

			if(!a_word_is_literal) {
				*a_word = *a_word & *b_word;
			}

			return cycles;
		case DCPU16_OPCODE_BOR:
			cycles += 1;

			if(!a_word_is_literal) {
				*a_word = *a_word | *b_word;
			}

			return cycles;
		case DCPU16_OPCODE_XOR:
			cycles += 1;

			if(!a_word_is_literal) {
				*a_word = *a_word ^ *b_word;
			}

			return cycles;
		case DCPU16_OPCODE_IFE:
			cycles += 2;
			
			if(*a_word != *b_word)
			{
				// Jump past the next instruction
				DCPU16_WORD w_n = ram[reg_pc];
				char opcode_n = w & 0xF;
				
				reg_pc++;

				char a_n = (w_n >> 4) & 0x3F;
				char b_n = (w_n >> 10) & 0x3F;

				DCPU16_WORD * b_word_n;
				DCPU16_WORD * a_word_n;
				dcpu16_get_pointer(a_n, -1, &a_word_n);
				dcpu16_get_pointer(b_n, -1, &b_word_n);

				cycles++;
			}

			return cycles;
		case DCPU16_OPCODE_IFN:
			cycles += 2;

			if(*a_word == *b_word)
			{
				// Jump past the next instruction
				DCPU16_WORD w_n = ram[reg_pc];
				char opcode_n = w & 0xF;
				
				reg_pc++;

				char a_n = (w_n >> 4) & 0x3F;
				char b_n = (w_n >> 10) & 0x3F;

				DCPU16_WORD * b_word_n;
				DCPU16_WORD * a_word_n;
				dcpu16_get_pointer(a_n, -1, &a_word_n);
				dcpu16_get_pointer(b_n, -1, &b_word_n);

				cycles++;
			}

			return cycles;
		case DCPU16_OPCODE_IFG:
			cycles += 2;

			if(*a_word < *b_word)
			{
				// Jump past the next instruction
				DCPU16_WORD w_n = ram[reg_pc];
				char opcode_n = w & 0xF;
				
				reg_pc++;

				char a_n = (w_n >> 4) & 0x3F;
				char b_n = (w_n >> 10) & 0x3F;

				DCPU16_WORD * b_word_n;
				DCPU16_WORD * a_word_n;
				dcpu16_get_pointer(a_n, -1, &a_word_n);
				dcpu16_get_pointer(b_n, -1, &b_word_n);

				cycles++;
			}

			return cycles;
		case DCPU16_OPCODE_IFB:
			cycles += 2;	

			if(*a_word & *b_word == 0)
			{
				// Jump past the next instruction
				DCPU16_WORD w_n = ram[reg_pc];
				char opcode_n = w & 0xF;
				
				reg_pc++;

				char a_n = (w_n >> 4) & 0x3F;
				char b_n = (w_n >> 10) & 0x3F;

				DCPU16_WORD * b_word_n;
				DCPU16_WORD * a_word_n;
				dcpu16_get_pointer(a_n, -1, &a_word_n);
				dcpu16_get_pointer(b_n, -1, &b_word_n);

				cycles++;
			}

			return cycles;
		};
	} else {
		// Non-basic
		char o = (w >> 4) & 0x3F;
		char a = (w >> 10) & 0x3F;

		DCPU16_WORD * a_word;
		cycles += dcpu16_get_pointer(a, 1, &a_word);

		switch(o) {
			case DCPU16_NON_BASIC_OPCODE_RESERVED_0:
				
				return cycles;
			case DCPU16_NON_BASIC_OPCODE_JSR_A:
				if(reg_sp <= 0)
				{
					// Fix out of bounds
					reg_sp = DCPU16_RAM_SIZE - 1 + reg_sp;
				} else {
					reg_sp--;
				}

				ram[reg_sp] = reg_pc;
				reg_pc = *a_word;	

				return cycles;
		};

	}
}

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
	reg_sp = 0;
	reg_o = 0;

	// Load the RAM
	memset(ram, 0 , sizeof(ram));

	if(ram_file != 0) {
		FILE * rf = fopen(ram_file, "r");
	
		if(!rf) {
			printf("couldn't load ram from file\n");
			return 0;
		}

		int n = fread((char *) ram, sizeof(ram) * sizeof(DCPU16_WORD), 1, rf);
		fclose(rf);

		printf("ram loaded from file: %s\n", ram_file);
	}

	return 1;
}

void dcpu16_enter_ram()
{
	printf("enter ram as hexadecimal (small letters) values, 16-bit at a time\n");
	printf("type -1 when you are done (rest of ram will be filled with zeroes)\n\n");

	int tmp;

	DCPU16_WORD word;
	DCPU16_WORD * ram_p = ram;

	while(1) {
		unsigned int ram_index = ram_p - ram;

		printf("%.4X:\t", ram_index);
		if(!scanf("%x", &tmp))
			break;
		
		if(tmp <= 0xFFFF && tmp >= 0) {
			word = (DCPU16_WORD) tmp;
			*ram_p = word;
			ram_p++;
		} else {
			break;
		}
	}

	putchar('\n');
}

void dcpu16_run() 
{
	while(reg_pc < 0xFFFF) {
		dcpu16_step();
	}
}

void dcpu16_run_debug()
{
	while(reg_pc < 0xFFFF) {
		char c = getchar();

		if(c == 's') {
			printf("stepping\n");
			printf("\tpc: %.4x\n", reg_pc);
			printf("\tinstruction:%.4x\n", ram[reg_pc]);
			printf("\tcompleted in %x cycles\n", dcpu16_step());
			printf("\tnew pc: %.4x\n", reg_pc);
		} else if(c == 'r') {
			dcpu16_print_registers();
		} else if(c == 'd') {
			unsigned int start, end;

			printf("ram dump start index: ");
			scanf("%x", &start);
			printf("ram dump end index: ");
			scanf("%x", &end);

			dcpu16_dump_ram(start, end);
		} else if(c == 'q') {
			return;
		}
	}
}



void main(int argc, char * argv[]) 
{
	char * ram_file = argv[1];

	printf("\nDCPU16 emulator v. 0.01\n\n");

	dcpu16_init(0);
	dcpu16_enter_ram();
	dcpu16_run_debug();
}
