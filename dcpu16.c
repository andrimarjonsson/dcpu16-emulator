#include <stdio.h>
#include <string.h>
#include "dcpu16.h"

DCPU16_WORD reg_a, reg_b, reg_c, reg_x, reg_y, reg_z, reg_i, reg_j, reg_pc, reg_sp, reg_o;
DCPU16_WORD * reg_pointers[] = { &reg_a, &reg_b, &reg_c, &reg_x, &reg_y, &reg_z, &reg_i, &reg_j };

DCPU16_WORD ram[DCPU16_RAM_SIZE];

/* Returns a pointer to the register with the index specified. NOTE: only works with register a, b, c, x, y, z, i and j. */
DCPU16_WORD * dcpu16_register_pointer(char index)
{
	return reg_pointers[index];
}

/* Returns the value of a register or somwhere in RAM. */
unsigned char dcpu16_get_pointer(unsigned char where, DCPU16_WORD * tmp_storage, DCPU16_WORD ** retval)
{
	if(where <= DCPU16_AB_VALUE_REG_J) {
		// 0x00-0x07 (value of register)
		*retval = dcpu16_register_pointer(where);

		return 0;
	} else if(where <= DCPU16_AB_VALUE_PTR_REG_J) {
		// 0x08-0x0f (value at address pointed to by register)
		*retval = &ram[*dcpu16_register_pointer(where - DCPU16_AB_VALUE_PTR_REG_A)];
		
		return 0;
	} else if(where <= DCPU16_AB_VALUE_PTR_REG_J_PLUS_WORD) {
		// 0x10-0x17 (value at address pointed to by the sum of the register and the next word)
		*retval = &ram[(DCPU16_WORD)(*dcpu16_register_pointer(where - DCPU16_AB_VALUE_PTR_REG_A_PLUS_WORD) + ram[reg_pc])];

		return 1;
	} else if(where >= 0x20 && where <= 0x3F) {
		// 0x20-0x3F (literal value)
		if(tmp_storage)
			*tmp_storage = where - 0x20;

		*retval = tmp_storage;
		
		return 0;
	}

	// The rest are handled individually
	switch(where) {
	case DCPU16_AB_VALUE_POP:
		*retval = &ram[reg_sp];
		reg_sp++;
		return 0;
	case DCPU16_AB_VALUE_PEEK:
		*retval = &ram[reg_sp];
		return 0;
	case DCPU16_AB_VALUE_PUSH:
		reg_sp--;
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

	*retval = 0;
	return 0;
}

/* Must be used when setting the value pointed to by a pointer returned from dpcu16_get_pointer (allows hardware mapped RAM). */
void dcpu16_set(DCPU16_WORD * where, DCPU16_WORD value)
{
	if(where == ram && where < ram + DCPU16_RAM_SIZE) {
		DCPU16_WORD address = where - ram;

		// Check for hardware mapped RAM here

		*where = value;
	} else {
		// Register
		*where = value;
	}
}

/* Must be used when getting the value pointed to by a pointer returned from dpcu16_get_pointer (allows hardware mapped RAM). */
DCPU16_WORD dcpu16_get(DCPU16_WORD * where)
{
	if(where == ram && where < ram + DCPU16_RAM_SIZE) {
		DCPU16_WORD address = where - ram;

		// Check for hardware mapped RAM here

		return *where;
	} else {
		// Register
		return *where;
	}
}

/* Returns 1 if v is a literal, else 0. */
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

/* Executes the next instruction, returns the number of cycles used. */
unsigned char dcpu16_step() 
{
	unsigned char cycles = 0;

	// Get the next instruction
	DCPU16_WORD w = ram[reg_pc];
	char opcode = w & 0xF;
	
	reg_pc++;

	if(opcode == DCPU16_OPCODE_NON_BASIC) {
		// Non-basic instruction
		char o = (w >> 4) & 0x3F;
		char a = (w >> 10) & 0x3F;

		// Temporary storage for embedded literal values
		DCPU16_WORD a_literal_tmp;

		// Get pointer to A
		DCPU16_WORD * a_word;
		cycles += dcpu16_get_pointer(a, &a_literal_tmp, &a_word);

		switch(o) {
		case DCPU16_NON_BASIC_OPCODE_RESERVED_0:
				
			return cycles;
		case DCPU16_NON_BASIC_OPCODE_JSR_A:
			reg_sp--;
			ram[reg_sp] = reg_pc;

			reg_pc = dcpu16_get(a_word);	

			return cycles;
		};

	} else {
		// Basic instruction
		char a = (w >> 4) & 0x3F;
		char b = (w >> 10) & 0x3F;

		// Temporary storage for embedded literal values
		DCPU16_WORD a_literal_tmp, b_literal_tmp;

		// Get pointer to A and B
		DCPU16_WORD * b_word;
		DCPU16_WORD * a_word;
		cycles += dcpu16_get_pointer(a, &a_literal_tmp, &a_word);
		cycles += dcpu16_get_pointer(b, &b_literal_tmp, &b_word);

		// Is A a literal?
		char a_literal = dcpu16_is_literal(a);

		switch(opcode) {
		case DCPU16_OPCODE_SET:
			cycles += 1;
			if(!a_literal)
				dcpu16_set(a_word, dcpu16_get(b_word));

			return cycles;
		case DCPU16_OPCODE_ADD:
			cycles += 2;
			if(!a_literal) {
				if((int) dcpu16_get(a_word) + (int) dcpu16_get(b_word) > 0xFFFF)
					reg_o = 1;
				else
					reg_o = 0;

				dcpu16_set(a_word, dcpu16_get(a_word) + dcpu16_get(b_word));
			}

			return cycles;
		case DCPU16_OPCODE_SUB:
			cycles += 2;
			if(!a_literal) {
				if((int) dcpu16_get(a_word) - (int) dcpu16_get(b_word) < 0) {
					reg_o = 0xFFFF;
				} else {
					reg_o = 0;
				}

				dcpu16_set(a_word, dcpu16_get(a_word) - dcpu16_get(b_word));
			}

			return cycles;
		case DCPU16_OPCODE_MUL:
			cycles += 2;

			if(!a_literal) {
				reg_o = ((dcpu16_get(a_word) * dcpu16_get(b_word)) >> 16) & 0xFFFF;
				dcpu16_set(a_word, dcpu16_get(a_word) * dcpu16_get(b_word));
			}

			return cycles;
		case DCPU16_OPCODE_DIV:
			cycles += 3;

			if(!a_literal) {
				if(dcpu16_get(b_word) == 0) {
					reg_a = 0;
					reg_o = 0;
				} else {
					reg_o = ((dcpu16_get(a_word) << 16) / dcpu16_get(b_word)) & 0xFFFF;
					dcpu16_set(a_word, dcpu16_get(a_word) / dcpu16_get(b_word));
				}
			}

			return cycles;
		case DCPU16_OPCODE_MOD:
			cycles += 3;

			if(!a_literal) {
				if(dcpu16_get(b_word) == 0) {
					reg_a = 0;
				} else {
					dcpu16_set(a_word, dcpu16_get(a_word) % dcpu16_get(b_word));
				}
			}

			return cycles;
		case DCPU16_OPCODE_SHL:
			cycles += 2;

			if(!a_literal) {
				reg_o = ((dcpu16_get(a_word) << dcpu16_get(b_word)) >> 16) & 0xFFFF;
				dcpu16_set(a_word, dcpu16_get(a_word) << dcpu16_get(b_word));
			}

			return cycles;
		case DCPU16_OPCODE_SHR:
			cycles += 2;

			if(!a_literal) {
				reg_o = ((dcpu16_get(a_word) << 16) >> dcpu16_get(b_word)) & 0xFFFF;
				dcpu16_set(a_word, dcpu16_get(a_word) >> dcpu16_get(b_word));
			}

			return cycles;
		case DCPU16_OPCODE_AND:
			cycles += 1;

			if(!a_literal) {
				dcpu16_set(a_word, dcpu16_get(a_word) & dcpu16_get(b_word));
			}

			return cycles;
		case DCPU16_OPCODE_BOR:
			cycles += 1;

			if(!a_literal) {
				dcpu16_set(a_word, dcpu16_get(a_word) | dcpu16_get(b_word));
			}

			return cycles;
		case DCPU16_OPCODE_XOR:
			cycles += 1;

			if(!a_literal) {
				dcpu16_set(a_word, dcpu16_get(a_word) ^ dcpu16_get(b_word));
			}

			return cycles;
		case DCPU16_OPCODE_IFE:
			cycles += 2;
			
			if(dcpu16_get(a_word) != dcpu16_get(b_word))
			{
				dcpu16_skip_next_instruction();
				cycles++;
			}

			return cycles;
		case DCPU16_OPCODE_IFN:
			cycles += 2;

			if(dcpu16_get(a_word) == dcpu16_get(b_word))
			{
				dcpu16_skip_next_instruction();
				cycles++;
			}

			return cycles;
		case DCPU16_OPCODE_IFG:
			cycles += 2;

			if(dcpu16_get(a_word) <= dcpu16_get(b_word))
			{
				dcpu16_skip_next_instruction();
				cycles++;
			}

			return cycles;
		case DCPU16_OPCODE_IFB:
			cycles += 2;	

			if(dcpu16_get(a_word) & dcpu16_get(b_word) == 0)
			{
				dcpu16_skip_next_instruction();
				cycles++;
			}

			return cycles;
		};
	}

	return cycles;
}

/* Displays the contents of the registers. */
void dcpu16_print_registers()
{
	printf("--------------------------------------------------------------\n");
	printf("a:%x b:%x c:%x x:%x y:%x z:%x i:%x j:%x pc:%x sp:%x o:%x\n", 
		reg_a, reg_b, reg_c, reg_x, reg_y, reg_z, reg_i, reg_j, reg_pc, reg_sp, reg_o);
	printf("--------------------------------------------------------------\n");
}

/* Prints the contents of the RAM. */
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

/* Initializes the emulator (sets register values and clears the RAM) */
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

/* Loads a program into the RAM, from a file. */
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
		"\tType 'r' to print the contents of the registers.\n"
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
			printf("pc: %.4x | instruction: %.4x | cycles: %d | pc afterwards: %.4x\t\n\n",
				pc_before, ram[pc_before], cycles, reg_pc);
			break;
		}
		case 'r':
			dcpu16_print_registers();
			putchar('\n');
			break;

		case 'd':
		{
			unsigned int d_start;
			unsigned int d_end;
			printf("\nRAM dump start index: ");
			scanf("%x", &d_start);
			printf("RAM dump end index: ");
			scanf("%x", &d_end);
			dcpu16_dump_ram(d_start, d_end);
			break;
		}
		};
	}
}

void dcpu16_run()
{
	while(!(ram[reg_pc] == (((0x20 + reg_pc) << 10) | 1) ||
	      ram[reg_pc] == 0x7DC1 && ram[(DCPU16_WORD)(reg_pc + 1)] == reg_pc)) {
		dcpu16_step();
	}

	printf("Infinite loop detected (probably reached end of code, yes this emulator is fast), emulator has been stopped.\n\n");
	printf("STATE:\n");
	dcpu16_print_registers();

	char c = -1;

	printf("\nWaiting for user input...\n"
		"\tType 'd' to display what's in the RAM.\n"
	       	"\tType 'q' to quit.\n\n");

	while(c != 'q') {
		// Get new user input
		c = getchar();

		if(c == 'd') {
			unsigned int d_start;
			unsigned int d_end;
			printf("\nRAM dump start index: ");
			scanf("%x", &d_start);
			printf("RAM dump end index: ");
			scanf("%x", &d_end);
			dcpu16_dump_ram(d_start, d_end);
		}
	}
}

void main(int argc, char * argv[]) 
{
	printf("\nDCPU16 emulator started\n\n");
	dcpu16_init();

	char debug_mode = 0;
	char ram_loaded = 0;

	for(int c = 1; c < argc; c++) {
		if(strcmp(argv[c], "-d") == 0) {
			debug_mode = 1;
		} else {
			char * ram_file = argv[c];
			dcpu16_load_ram(ram_file);
			ram_loaded = 1;
		}
	}

	if(!ram_loaded)
		dcpu16_enter_ram();

	if(debug_mode)
		dcpu16_run_debug();
	else
		dcpu16_run();
}
