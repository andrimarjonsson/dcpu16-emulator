#include <stdio.h>
#include <string.h>
#include "dcpu16.h"

/* Returns a pointer to the register with the index specified. 
   NOTE: this can work with any register, but it is intended to
   be used for registers a, b, c, x, y, z, i and j.  Addressing
   other registers using this function is not recommended. */
DCPU16_WORD * dcpu16_register_pointer(dcpu16_t *computer, char index)
{
	return &computer->registers[index];
}

/* Returns the value of a register or somwhere in RAM. */
unsigned char dcpu16_get_pointer(dcpu16_t *computer, unsigned char where, DCPU16_WORD * tmp_storage, DCPU16_WORD ** retval)
{
	if(where <= DCPU16_AB_VALUE_REG_J) {
		// 0x00-0x07 (value of register)
		*retval = dcpu16_register_pointer(computer, where);

		return 0;
	} else if(where <= DCPU16_AB_VALUE_PTR_REG_J) {
		// 0x08-0x0f (value at address pointed to by register)
		*retval = &computer->ram[*dcpu16_register_pointer(computer, where - DCPU16_AB_VALUE_PTR_REG_A)];
		
		return 0;
	} else if(where <= DCPU16_AB_VALUE_PTR_REG_J_PLUS_WORD) {
		// 0x10-0x17 (value at address pointed to by the sum of the register and the next word)
		*retval = &computer->ram[(DCPU16_WORD)(*dcpu16_register_pointer(computer, where - DCPU16_AB_VALUE_PTR_REG_A_PLUS_WORD) + computer->ram[computer->registers[DCPU16_INDEX_REG_PC]])];

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
		*retval = &computer->ram[computer->registers[DCPU16_INDEX_REG_SP]];
		computer->registers[DCPU16_INDEX_REG_SP]++;
		return 0;
	case DCPU16_AB_VALUE_PEEK:
		*retval = &computer->ram[computer->registers[DCPU16_INDEX_REG_SP]];
		return 0;
	case DCPU16_AB_VALUE_PUSH:
		computer->registers[DCPU16_INDEX_REG_SP]--;
		*retval = &computer->ram[computer->registers[DCPU16_INDEX_REG_SP]];
		return 0;
	case DCPU16_AB_VALUE_REG_SP:
		*retval = &computer->registers[DCPU16_INDEX_REG_SP];
		return 0;
	case DCPU16_AB_VALUE_REG_PC:
		*retval = &computer->registers[DCPU16_INDEX_REG_PC];
		return 0;
	case DCPU16_AB_VALUE_REG_O:
		*retval = &computer->registers[DCPU16_INDEX_REG_O];
		return 0;
	case DCPU16_AB_VALUE_PTR_WORD:
		*retval = &computer->ram[computer->ram[computer->registers[DCPU16_INDEX_REG_PC]]];
		computer->registers[DCPU16_INDEX_REG_PC]++;
		return 1;
	case DCPU16_AB_VALUE_WORD:
		*retval = &computer->ram[computer->registers[DCPU16_INDEX_REG_PC]];
		computer->registers[DCPU16_INDEX_REG_PC]++;
		return 1;
	};

	*retval = 0;
	return 0;
}

/* Must be used when setting the value pointed to by a pointer returned from dpcu16_get_pointer (allows hardware mapped RAM). */
void dcpu16_set(dcpu16_t * computer, DCPU16_WORD * where, DCPU16_WORD value)
{
	if(where >= computer->ram && where < computer->ram + DCPU16_RAM_SIZE) {
		DCPU16_WORD address = where - computer->ram;

		// Check for hardware mapped RAM here

		*where = value;
	} else {
		// Register
		*where = value;
	}
}

/* Must be used when getting the value pointed to by a pointer returned from dpcu16_get_pointer (allows hardware mapped RAM). */
DCPU16_WORD dcpu16_get(dcpu16_t * computer, DCPU16_WORD * where)
{
	if(where >= computer->ram && where < computer->ram + DCPU16_RAM_SIZE) {
		DCPU16_WORD address = where - computer->ram;

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
void dcpu16_skip_next_instruction(dcpu16_t *computer)
{
	DCPU16_WORD w = computer->ram[computer->registers[DCPU16_INDEX_REG_PC]];
	char opcode = w & 0xF;
				
	computer->registers[DCPU16_INDEX_REG_PC]++;

	if(opcode != DCPU16_OPCODE_NON_BASIC) {
		char a = (w >> 4) & 0x3F;
		char b = (w >> 10) & 0x3F;

		DCPU16_WORD * b_word;
		DCPU16_WORD * a_word;
		dcpu16_get_pointer(computer, a, 0, &a_word);
		dcpu16_get_pointer(computer, b, 0, &b_word);
	} else {
		char a = (w >> 10) & 0x3F;

		DCPU16_WORD * a_word;
		dcpu16_get_pointer(computer, a, 0, &a_word);
	}

}

/* Executes the next instruction, returns the number of cycles used. */
unsigned char dcpu16_step(dcpu16_t *computer) 
{
	unsigned char cycles = 0;

	// Get the next instruction
	DCPU16_WORD w = computer->ram[computer->registers[DCPU16_INDEX_REG_PC]];
	char opcode = w & 0xF;
	
	computer->registers[DCPU16_INDEX_REG_PC]++;

	if(opcode == DCPU16_OPCODE_NON_BASIC) {
		// Non-basic instruction
		char o = (w >> 4) & 0x3F;
		char a = (w >> 10) & 0x3F;

		// Temporary storage for embedded literal values
		DCPU16_WORD a_literal_tmp;

		// Get pointer to A
		DCPU16_WORD * a_word;
		cycles += dcpu16_get_pointer(computer, a, &a_literal_tmp, &a_word);

		switch(o) {
		case DCPU16_NON_BASIC_OPCODE_RESERVED_0:
				
			return cycles;
		case DCPU16_NON_BASIC_OPCODE_JSR_A:
			computer->registers[DCPU16_INDEX_REG_SP]--;
			computer->ram[computer->registers[DCPU16_INDEX_REG_SP]] = computer->registers[DCPU16_INDEX_REG_PC];

			computer->registers[DCPU16_INDEX_REG_PC] = dcpu16_get(computer, a_word);	

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
		cycles += dcpu16_get_pointer(computer, a, &a_literal_tmp, &a_word);
		cycles += dcpu16_get_pointer(computer, b, &b_literal_tmp, &b_word);

		// Is A a literal?
		char a_literal = dcpu16_is_literal(a);

		switch(opcode) {
		case DCPU16_OPCODE_SET:
			cycles += 1;
			if(!a_literal)
				dcpu16_set(computer, a_word, dcpu16_get(computer, b_word));

			return cycles;
		case DCPU16_OPCODE_ADD:
			cycles += 2;
			if(!a_literal) {
				if((int) dcpu16_get(computer, a_word) + (int) dcpu16_get(computer, b_word) > 0xFFFF)
					computer->registers[DCPU16_INDEX_REG_O] = 1;
				else
					computer->registers[DCPU16_INDEX_REG_O] = 0;

				dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) + dcpu16_get(computer, b_word));
			}

			return cycles;
		case DCPU16_OPCODE_SUB:
			cycles += 2;
			if(!a_literal) {
				if((int) dcpu16_get(computer, a_word) - (int) dcpu16_get(computer, b_word) < 0) {
					computer->registers[DCPU16_INDEX_REG_O] = 0xFFFF;
				} else {
					computer->registers[DCPU16_INDEX_REG_O] = 0;
				}

				dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) - dcpu16_get(computer, b_word));
			}

			return cycles;
		case DCPU16_OPCODE_MUL:
			cycles += 2;

			if(!a_literal) {
				computer->registers[DCPU16_INDEX_REG_O] = ((dcpu16_get(computer, a_word) * dcpu16_get(computer, b_word)) >> 16) & 0xFFFF;
				dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) * dcpu16_get(computer, b_word));
			}

			return cycles;
		case DCPU16_OPCODE_DIV:
			cycles += 3;

			if(!a_literal) {
				if(dcpu16_get(computer, b_word) == 0) {
					computer->registers[DCPU16_INDEX_REG_A] = 0;
					computer->registers[DCPU16_INDEX_REG_O] = 0;
				} else {
					computer->registers[DCPU16_INDEX_REG_O] = ((dcpu16_get(computer, a_word) << 16) / dcpu16_get(computer, b_word)) & 0xFFFF;
					dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) / dcpu16_get(computer, b_word));
				}
			}

			return cycles;
		case DCPU16_OPCODE_MOD:
			cycles += 3;

			if(!a_literal) {
				if(dcpu16_get(computer, b_word) == 0) {
					computer->registers[DCPU16_INDEX_REG_A] = 0;
				} else {
					dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) % dcpu16_get(computer, b_word));
				}
			}

			return cycles;
		case DCPU16_OPCODE_SHL:
			cycles += 2;

			if(!a_literal) {
				computer->registers[DCPU16_INDEX_REG_O] = ((dcpu16_get(computer, a_word) << dcpu16_get(computer, b_word)) >> 16) & 0xFFFF;
				dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) << dcpu16_get(computer, b_word));
			}

			return cycles;
		case DCPU16_OPCODE_SHR:
			cycles += 2;

			if(!a_literal) {
				computer->registers[DCPU16_INDEX_REG_O] = ((dcpu16_get(computer, a_word) << 16) >> dcpu16_get(computer, b_word)) & 0xFFFF;
				dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) >> dcpu16_get(computer, b_word));
			}

			return cycles;
		case DCPU16_OPCODE_AND:
			cycles += 1;

			if(!a_literal) {
				dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) & dcpu16_get(computer, b_word));
			}

			return cycles;
		case DCPU16_OPCODE_BOR:
			cycles += 1;

			if(!a_literal) {
				dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) | dcpu16_get(computer, b_word));
			}

			return cycles;
		case DCPU16_OPCODE_XOR:
			cycles += 1;

			if(!a_literal) {
				dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) ^ dcpu16_get(computer, b_word));
			}

			return cycles;
		case DCPU16_OPCODE_IFE:
			cycles += 2;
			
			if(dcpu16_get(computer, a_word) != dcpu16_get(computer, b_word))
			{
				dcpu16_skip_next_instruction(computer);
				cycles++;
			}

			return cycles;
		case DCPU16_OPCODE_IFN:
			cycles += 2;

			if(dcpu16_get(computer, a_word) == dcpu16_get(computer, b_word))
			{
				dcpu16_skip_next_instruction(computer);
				cycles++;
			}

			return cycles;
		case DCPU16_OPCODE_IFG:
			cycles += 2;

			if(dcpu16_get(computer, a_word) <= dcpu16_get(computer, b_word))
			{
				dcpu16_skip_next_instruction(computer);
				cycles++;
			}

			return cycles;
		case DCPU16_OPCODE_IFB:
			cycles += 2;	

			if(dcpu16_get(computer, a_word) & dcpu16_get(computer, b_word) == 0)
			{
				dcpu16_skip_next_instruction(computer);
				cycles++;
			}

			return cycles;
		};
	}

	return cycles;
}

/* Displays the contents of the registers. */
void dcpu16_print_registers(dcpu16_t *computer)
{
	printf("--------------------------------------------------------------\n");
	printf("a:%x b:%x c:%x x:%x y:%x z:%x i:%x j:%x pc:%x sp:%x o:%x\n", 
		computer->registers[DCPU16_INDEX_REG_A], computer->registers[DCPU16_INDEX_REG_B], computer->registers[DCPU16_INDEX_REG_C], computer->registers[DCPU16_INDEX_REG_X], computer->registers[DCPU16_INDEX_REG_Y], computer->registers[DCPU16_INDEX_REG_Z], computer->registers[DCPU16_INDEX_REG_I], computer->registers[DCPU16_INDEX_REG_J], computer->registers[DCPU16_INDEX_REG_PC], computer->registers[DCPU16_INDEX_REG_SP], computer->registers[DCPU16_INDEX_REG_O]);
	printf("--------------------------------------------------------------\n");
}

/* Prints the contents of the RAM. */
void dcpu16_dump_ram(dcpu16_t *computer, unsigned int start, unsigned int end)
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
			DCPU16_WORD w = computer->ram[start + i];
			printf("%.4x ", w);
		}

		putchar('\n');
	}

	putchar('\n');
}

/* Initializes the emulator (sets register values and clears the RAM) */
void dcpu16_init(dcpu16_t *computer)
{
	// Set registers
	computer->registers[DCPU16_INDEX_REG_A] = 0;
	computer->registers[DCPU16_INDEX_REG_B] = 0;
	computer->registers[DCPU16_INDEX_REG_C] = 0;
	computer->registers[DCPU16_INDEX_REG_X] = 0;
	computer->registers[DCPU16_INDEX_REG_Y] = 0;
	computer->registers[DCPU16_INDEX_REG_Z] = 0;
	computer->registers[DCPU16_INDEX_REG_I] = 0;
	computer->registers[DCPU16_INDEX_REG_J] = 0;

	computer->registers[DCPU16_INDEX_REG_PC] = 0;
	computer->registers[DCPU16_INDEX_REG_SP] = 0;
	computer->registers[DCPU16_INDEX_REG_O] = 0;

	// Clear the RAM
	memset(computer->ram, 0 , sizeof(computer->ram));
}

/* Loads a program into the RAM, from a file. */
void dcpu16_load_ram(dcpu16_t *computer, char * ram_file)
{
	FILE * rf = fopen(ram_file, "r");

	// Check for errors
	if(!rf) {
		printf("Unable to open ram file.\n");
		return;
	}

	// Load hexadecimal integers from the file
	DCPU16_WORD * ram_p = computer->ram;

	while(!feof(rf)) {
		DCPU16_WORD w;
		if(fscanf(rf, "%hx", &w) > 0) {
			*ram_p = w;
			ram_p++;
		}

		// Check if there is more RAM
		if(ram_p == computer->ram + DCPU16_RAM_SIZE) {
			printf("Couldn't load all RAM data from file, not enough RAM to store it in.");
			break;
		}
			
	}
	
	DCPU16_WORD bytesLoaded = ram_p - computer->ram;
	printf("Loaded %d bytes into RAM\n", (int)bytesLoaded);

	fclose(rf);
}

void dcpu16_enter_ram(dcpu16_t *computer)
{
	printf("ENTER RAM CONTENT MANUALLY\n");
	printf("Expects lower case letters, hexadecimal 16-bit integers.\n\n");
	printf("Type ENTER without entering a number when you are done."
		"The rest of the RAM will be filled with zeroes.\n\n");

	DCPU16_WORD word;
	DCPU16_WORD * ram_p = computer->ram;

	char tmp[64];
	memset(tmp, 0, sizeof(tmp));

	for(;;) {
		unsigned int ram_index = ram_p - computer->ram;
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

void dcpu16_run_debug(dcpu16_t *computer)
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
			int pc_before = computer->registers[DCPU16_INDEX_REG_PC];
			int cycles = dcpu16_step(computer);
			printf("pc: %.4x | instruction: %.4x | cycles: %d | pc afterwards: %.4x\t\n\n",
				pc_before, computer->ram[pc_before], cycles, computer->registers[DCPU16_INDEX_REG_PC]);
			break;
		}
		case 'r':
			dcpu16_print_registers(computer);
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
			dcpu16_dump_ram(computer, d_start, d_end);
			break;
		}
		};
	}
}

void dcpu16_run(dcpu16_t *computer)
{
	while(!(computer->ram[computer->registers[DCPU16_INDEX_REG_PC]] == (((0x20 + computer->registers[DCPU16_INDEX_REG_PC]) << 10) | 1) ||
	      computer->ram[computer->registers[DCPU16_INDEX_REG_PC]] == 0x7DC1 && computer->ram[(DCPU16_WORD)(computer->registers[DCPU16_INDEX_REG_PC] + 1)] == computer->registers[DCPU16_INDEX_REG_PC])) {
		dcpu16_step(computer);
	}

	printf("Infinite loop detected (probably reached end of code, yes this emulator is fast), emulator has been stopped.\n\n");
	printf("STATE:\n");
	dcpu16_print_registers(computer);

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
			dcpu16_dump_ram(computer, d_start, d_end);
		}
	}
}

int main(int argc, char * argv[]) 
{
	printf("\nDCPU16 emulator started\n\n");
	dcpu16_t computerOnTheStack;
	dcpu16_t *computer = &computerOnTheStack;
	dcpu16_init(computer);
	
	char debug_mode = 0;
	char ram_loaded = 0;

	for(int c = 1; c < argc; c++) {
		if(strcmp(argv[c], "-d") == 0) {
			debug_mode = 1;
		} else {
			char * ram_file = argv[c];
			dcpu16_load_ram(computer, ram_file);
			ram_loaded = 1;
		}
	}

	if(!ram_loaded)
		dcpu16_enter_ram(computer);

	if(debug_mode)
		dcpu16_run_debug(computer);
	else
		dcpu16_run(computer);
	
	return 0;
}
