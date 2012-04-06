#include <stdio.h>
#include <string.h>
#include "dcpu16.h"

DCPU16_WORD reg_a, reg_b, reg_c, reg_x, reg_y, reg_z, reg_i, reg_j, reg_pc, reg_sp, reg_o;
DCPU16_WORD ram[DCPU16_RAM_SIZE];

void dcpu16_print_registers()
{
	printf("--------------------------------------------------------------\n");
	printf("a:%x b:%x c:%x x:%x y:%x z:%x i:%x j:%x pc:%x sp:%x o:%x\n", 
		reg_a, reg_b, reg_c, reg_x, reg_y, reg_z, reg_i, reg_j, reg_pc, reg_sp, reg_o);
	printf("--------------------------------------------------------------\n");
}

void dcpu16_dump_ram(unsigned int start, unsigned int end)
{
	if(end >= DCPU16_RAM_SIZE - 8)
		end = DCPU16_RAM_SIZE - 9;

	if(start % 8 != 0) {
		int tmp = start / 8;
		start = tmp * 8;
	}

	if(end % 8 != 0) {
		int tmp = end / 8;
		end = (tmp + 1) * 8;
	}

	printf("\nRAM DUMP\n");

	for(; start <= end; start+=8) {
		printf("%.4x:", start);

		for(int i = 0; i < 8; i++) {
			DCPU16_WORD w = ram[start + i];
			printf("%.4x ", w);
		}

		putchar('\n');
	}

	putchar('\n');
}

/* Returns the actual address (checks if the address is out of bounds, wraps around if it is). */
int dcpu16_ram_address(int address)
{
	if(address >= DCPU16_RAM_SIZE)
		address = address % DCPU16_RAM_SIZE; 
}

/* Subtracts one from the word (accounting for wraparound while the word is a RAM address) .*/
int dcpu16_subtract_one_ram_address(DCPU16_WORD * w)
{
	if(*w == 0) {
		*w = DCPU16_RAM_SIZE - 1;
	} else {
		*w = *w - 1;
	}
}


int dcpu16_get_pointer(char v, DCPU16_WORD * literal_tmp_storage, DCPU16_WORD ** retval)
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
			*retval = &ram[dcpu16_ram_address(reg_a + ram[dcpu16_ram_address(reg_pc)])];
			reg_pc++;
			return 1;
		case DCPU16_AB_VALUE_PTR_REG_B_PLUS_WORD:
			*retval = &ram[dcpu16_ram_address(reg_b + ram[dcpu16_ram_address(reg_pc)])];
			reg_pc++;
			return 1;
		case DCPU16_AB_VALUE_PTR_REG_C_PLUS_WORD:
			*retval = &ram[dcpu16_ram_address(reg_c + ram[dcpu16_ram_address(reg_pc)])];
			reg_pc++;
			return 1;
		case DCPU16_AB_VALUE_PTR_REG_X_PLUS_WORD:
			*retval = &ram[dcpu16_ram_address(reg_c + ram[dcpu16_ram_address(reg_pc)])];
			reg_pc++;
			return 1;
		case DCPU16_AB_VALUE_PTR_REG_Y_PLUS_WORD:
			*retval = &ram[dcpu16_ram_address(reg_y + ram[dcpu16_ram_address(reg_pc)])];
			reg_pc++;
			return 1;
		case DCPU16_AB_VALUE_PTR_REG_Z_PLUS_WORD:
			*retval = &ram[dcpu16_ram_address(reg_z + ram[dcpu16_ram_address(reg_pc)])];
			reg_pc++;
			return 1;
		case DCPU16_AB_VALUE_PTR_REG_I_PLUS_WORD:
			*retval = &ram[dcpu16_ram_address(reg_i + ram[dcpu16_ram_address(reg_pc)])];
			reg_pc++;
			return 1;
		case DCPU16_AB_VALUE_PTR_REG_J_PLUS_WORD:
			*retval = &ram[dcpu16_ram_address(reg_j + ram[dcpu16_ram_address(reg_pc)])];
			reg_pc++;
			return 1;
		case DCPU16_AB_VALUE_POP:
			*retval = &ram[reg_sp];
			reg_sp++;
			reg_sp = dcpu16_ram_address(reg_sp);
			return 0;
		case DCPU16_AB_VALUE_PEEK:
			*retval = &ram[reg_sp];
			return 0;
		case DCPU16_AB_VALUE_PUSH:
			dcpu16_subtract_one_ram_address(&reg_sp);
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
			*retval = &ram[dcpu16_ram_address(ram[dcpu16_ram_address(reg_pc)])];
			reg_pc++;
			return 1;
		case DCPU16_AB_VALUE_WORD:
			*retval = &ram[dcpu16_ram_address(reg_pc)];
			reg_pc++;
			return 1;
	};

	// Handle embedded literal values
	if(v >= 0x20 && v <= 0x3F) {
		if(literal_tmp_storage)
			*literal_tmp_storage = v - 0x20;

		*retval = literal_tmp_storage;
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

/* Skips the next instruction (advances PC). */
void dcpu16_skip_next_instruction()
{
	DCPU16_WORD w = ram[reg_pc];
	char opcode = w & 0xF;
				
	reg_pc++;

	if(opcode != DCPU16_OPCODE_NON_BASIC) {
		char a = (w >> 4) & 0x3F;
		char b = (w >> 10) & 0x3F;

		DCPU16_WORD * b_word;
		DCPU16_WORD * a_word;
		dcpu16_get_pointer(a, 0, &a_word);
		dcpu16_get_pointer(b, 0, &b_word);
	} else {
		char a = (w >> 10) & 0x3F;

		DCPU16_WORD * a_word;
		dcpu16_get_pointer(a, 0, &a_word);
	}

}

int dcpu16_step() 
{
	DCPU16_WORD w = ram[reg_pc];
	char opcode = w & 0xF;
	int cycles = 0;

	// Increase PC (opcode)
	reg_pc++;

	if(opcode != DCPU16_OPCODE_NON_BASIC) {
		// Basic instructions

		char a = (w >> 4) & 0x3F;
		char b = (w >> 10) & 0x3F;

		// Temporary storage for embedded literal values
		DCPU16_WORD a_literal_tmp, b_literal_tmp;

		// Pointer to a and b values
		DCPU16_WORD * b_word;
		DCPU16_WORD * a_word;
		cycles += dcpu16_get_pointer(a, &a_literal_tmp, &a_word);
		cycles += dcpu16_get_pointer(b, &b_literal_tmp, &b_word);

		// Is A a literal?
		char a_is_literal = dcpu16_is_literal(a);

		switch(opcode) {
		case DCPU16_OPCODE_SET:
			cycles += 1;

			if(!a_is_literal) {
				*a_word = *b_word;
			}

			return cycles;
		case DCPU16_OPCODE_ADD:
			cycles += 2;

			if(!a_is_literal) {
				if((int) *a_word + (int) *b_word > 0xFFFF) {
					reg_o = 1;
				} else {
					reg_o = 0;
				}

				*a_word = *a_word + *b_word;
			}

			return cycles;
		case DCPU16_OPCODE_SUB:
			cycles += 2;

			if(!a_is_literal) {
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

			if(!a_is_literal) {
				reg_o = ((*a_word * *b_word) >> 16) & 0xFFFF;
				*a_word = *a_word * *b_word;
			}

			return cycles;
		case DCPU16_OPCODE_DIV:
			cycles += 3;

			if(!a_is_literal) {
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

			if(!a_is_literal) {
				if(*b_word == 0) {
					reg_a = 0;
				} else {
					*a_word = *a_word % *b_word;
				}
			}

			return cycles;
		case DCPU16_OPCODE_SHL:
			cycles += 2;

			if(!a_is_literal) {
				reg_o = ((*a_word << *b_word) >> 16) & 0xFFFF;
				*a_word = *a_word << *b_word;
			}

			return cycles;
		case DCPU16_OPCODE_SHR:
			cycles += 2;

			if(!a_is_literal) {
				reg_o = ((*a_word << 16) >> *b_word) & 0xFFFF;
				*a_word = *a_word >> *b_word;
			}

			return cycles;
		case DCPU16_OPCODE_AND:
			cycles += 1;

			if(!a_is_literal) {
				*a_word = *a_word & *b_word;
			}

			return cycles;
		case DCPU16_OPCODE_BOR:
			cycles += 1;

			if(!a_is_literal) {
				*a_word = *a_word | *b_word;
			}

			return cycles;
		case DCPU16_OPCODE_XOR:
			cycles += 1;

			if(!a_is_literal) {
				*a_word = *a_word ^ *b_word;
			}

			return cycles;
		case DCPU16_OPCODE_IFE:
			cycles += 2;
			
			if(*a_word != *b_word)
			{
				dcpu16_skip_next_instruction();
				cycles++;
			}

			return cycles;
		case DCPU16_OPCODE_IFN:
			cycles += 2;

			if(*a_word == *b_word)
			{
				dcpu16_skip_next_instruction();
				cycles++;
			}

			return cycles;
		case DCPU16_OPCODE_IFG:
			cycles += 2;

			if(*a_word <= *b_word)
			{
				dcpu16_skip_next_instruction();
				cycles++;
			}

			return cycles;
		case DCPU16_OPCODE_IFB:
			cycles += 2;	

			if(*a_word & *b_word == 0)
			{
				dcpu16_skip_next_instruction();
				cycles++;
			}

			return cycles;
		};
	} else {
		// Non-basic instructions
		char o = (w >> 4) & 0x3F;
		char a = (w >> 10) & 0x3F;

		// Temporary storage for embedded literal values
		DCPU16_WORD a_literal_tmp;

		DCPU16_WORD * a_word;
		cycles += dcpu16_get_pointer(a, &a_literal_tmp, &a_word);

		switch(o) {
			case DCPU16_NON_BASIC_OPCODE_RESERVED_0:
				
				return cycles;
			case DCPU16_NON_BASIC_OPCODE_JSR_A:
				dcpu16_subtract_one_ram_address(&reg_sp);

				ram[reg_sp] = reg_pc;
				reg_pc = *a_word;	

				return cycles;
		};

	}
}

void dcpu16_init()
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

	// Clear the RAM
	memset(ram, 0 , sizeof(ram));
}

void dcpu16_load_ram(char * ram_file)
{
	FILE * rf = fopen(ram_file, "r");

	// Check for errors
	if(!rf) {
		printf("Unable to open ram file.\n");
		return;
	}

	// Load hexadecimal integers from the file
	DCPU16_WORD * ram_p = ram;

	while(!feof(rf)) {
		DCPU16_WORD w;
		if(fscanf(rf, "%hx", &w) > 0) {
			*ram_p = w;
			ram_p++;
		}

		// Check if there is more RAM
		if(ram_p == ram + DCPU16_RAM_SIZE) {
			printf("Couldn't load all RAM data from file, not enough RAM to store it in.");
			break;
		}
			
	}

	fclose(rf);
}

void dcpu16_enter_ram()
{
	printf("ENTER RAM CONTENT MANUALLY\n");
	printf("Expects lower case letters, hexadecimal 16-bit integers.\n\n");
	printf("Type ENTER without entering a number when you are done."
		"The rest of the RAM will be filled with zeroes.\n\n");

	DCPU16_WORD word;
	DCPU16_WORD * ram_p = ram;

	char tmp[64];
	memset(tmp, 0, sizeof(tmp));

	for(;;) {
		unsigned int ram_index = ram_p - ram;
		printf("%.4x:\t", ram_index);

		// Read input as a string
		gets(tmp);
		tmp[63] = 0;

		if(!strcmp(tmp, ""))
			break;

		// Convert to unsigned short
		if(!sscanf(tmp, "%hx", &word))
			break;

		*ram_p = word;
		ram_p++;
	}

	putchar('\n');
}

void dcpu16_run_debug()
{

	printf("You are now emulating in debug mode.\n"
		"\tType 's' to execute the next instruction.\n"
		"\tType 'r' to print the content of the registers.\n"
		"\tType 'd' to display what's in the RAM.\n"
		"\tType 'q' to quit.\n\n");

	char c;

	while(c != 'q') {
		// Get new user input
		c = getchar();

		switch(c) {
		case 's':
		{
			int pc_before = reg_pc;
			int cycles = dcpu16_step();
			printf("pc: %.4x | instruction: %.4x | cycles: %d | pc afterwards: %.4x\t\n",
				pc_before, ram[pc_before], cycles, reg_pc);
			break;
		}
		case 'r':
			dcpu16_print_registers();
			break;

		case 'd':
		{
			unsigned int d_start;
			unsigned int d_end;
			printf("RAM dump start index: ");
			scanf("%x", &d_start);
			printf("RAM dump end index: ");
			scanf("%x", &d_end);
			dcpu16_dump_ram(d_start, d_end);
			break;
		}
		};
	}
}

void main(int argc, char * argv[]) 
{
	printf("\nDCPU16 emulator\n\n");
	
	dcpu16_init();

	if(argc > 1) {
		char * ram_file = argv[1];
		dcpu16_load_ram(ram_file);

	} else {
		dcpu16_enter_ram();
	}
	
	dcpu16_run_debug();
}
