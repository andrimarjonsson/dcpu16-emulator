#include <stdio.h>
#include <string.h>
#define __need_struct_timeval 1
#include <sys/time.h>
#include "dcpu16.h"

/* Must be used when setting the value of ANY register or any RAM of the emulated computer. */
static inline void dcpu16_set(dcpu16_t *computer, DCPU16_WORD *where, DCPU16_WORD value)
{
	if(where >= computer->ram && where < computer->ram + DCPU16_RAM_SIZE) {	// RAM
		// Calculate the RAM address
		DCPU16_WORD ram_address = where - computer->ram;

		// Check for hardware mapped RAM

		// Call the callback function if address was not hardware mapped
		if(computer->callback.unmapped_ram_changed)
			computer->callback.unmapped_ram_changed(ram_address, value);

	} else if(where >= computer->registers && where < computer->registers + DCPU16_REGISTER_COUNT) {	// Register
		// Call the callback function
		if(computer->callback.register_changed)
			computer->callback.register_changed(where - computer->registers, value);
	}
	
	// Set the value
	*where = value;
}

/* Instead of having to call dcpu16_set every time PC needs to increase, we can increase it directly.
   Partly because the callback for PC shouldn't be called every time when it's value is changed.
   E.g. when parsing an instruction that is more than one word long the callback would be called multiple times
   if we would use dcpu16_set. */
static inline void dcpu16_pc_callback(dcpu16_t *computer)
{	
	computer->registers[DCPU16_INDEX_REG_PC]++;

	if(computer->callback.register_changed)
		computer->callback.register_changed(DCPU16_INDEX_REG_PC, computer->registers[DCPU16_INDEX_REG_PC]);
}

/* Instead of having to call dcpu16_set every time SP needs to increase, we can call this function (should be faster). */
static inline void dcpu16_increase_sp(dcpu16_t *computer)
{
	computer->registers[DCPU16_INDEX_REG_SP]++;

	if(computer->callback.register_changed)
		computer->callback.register_changed(DCPU16_INDEX_REG_SP, computer->registers[DCPU16_INDEX_REG_SP]);
}

/* Instead of having to call dcpu16_set every time SP needs to decrease, we can call this function (should be faster). */
static inline void dcpu16_decrease_sp(dcpu16_t *computer)
{
	computer->registers[DCPU16_INDEX_REG_SP]--;

	if(computer->callback.register_changed)
		computer->callback.register_changed(DCPU16_INDEX_REG_SP, computer->registers[DCPU16_INDEX_REG_SP] - 1);
}

/* Returns a pointer to the register with the index specified. 
   NOTE: this can work with any register, but it is intended to
   be used for registers a, b, c, x, y, z, i and j.  Addressing
   other registers using this function is not recommended. */
static inline DCPU16_WORD *dcpu16_register_pointer(dcpu16_t *computer, char index)
{
	return &computer->registers[index];
}

/* Sets *retval to point to a register or a DCPU16_WORD in RAM. Returns the number of cycles it took to look it up. */
unsigned char dcpu16_get_pointer(dcpu16_t *computer, unsigned char where, DCPU16_WORD *tmp_storage, DCPU16_WORD **retval)
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
		*retval = &computer->ram[(DCPU16_WORD)(*dcpu16_register_pointer(computer, where - DCPU16_AB_VALUE_PTR_REG_A_PLUS_WORD) +
			computer->ram[computer->registers[DCPU16_INDEX_REG_PC]])];
		computer->registers[DCPU16_INDEX_REG_PC]++;
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
		dcpu16_increase_sp(computer);
		return 0;
	case DCPU16_AB_VALUE_PEEK:
		*retval = &computer->ram[computer->registers[DCPU16_INDEX_REG_SP]];
		return 0;
	case DCPU16_AB_VALUE_PUSH:
		dcpu16_decrease_sp(computer);
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

/* Must be used when getting the value of ANY register or any RAM of the emulated computer. */
static inline DCPU16_WORD dcpu16_get(dcpu16_t *computer, DCPU16_WORD *where)
{
	// NOTE: This function might be useless. Not sure if it is needed for getting keyboard input later.

	return *where;
}

/* Returns true if v is a literal (v is expected to be a 6-bit AB value). */
static inline char dcpu16_is_literal(char v)
{
	if(v == DCPU16_AB_VALUE_WORD || (v >= 0x20 && v <= 0x3F))
		return 1;

	return 0;
}

/* Skips the next instruction (advances PC). */
void dcpu16_skip_next_instruction(dcpu16_t *computer)
{
	// Parse the instruction but don't execute it
	DCPU16_WORD w = computer->ram[computer->registers[DCPU16_INDEX_REG_PC]];
	char opcode = w & 0xF;
	computer->registers[DCPU16_INDEX_REG_PC]++;

	if(opcode != DCPU16_OPCODE_NON_BASIC) {
		char a = (w >> 4) & 0x3F;
		char b = (w >> 10) & 0x3F;

		DCPU16_WORD *b_word;
		DCPU16_WORD *a_word;
		dcpu16_get_pointer(computer, a, 0, &a_word);
		dcpu16_get_pointer(computer, b, 0, &b_word);
	} else {
		char a = (w >> 10) & 0x3F;

		DCPU16_WORD *a_word;
		dcpu16_get_pointer(computer, a, 0, &a_word);
	}

	// Call the PC callback
	dcpu16_pc_callback(computer);
}

/* Executes the next instruction, returns the number of cycles used. */
unsigned char dcpu16_step(dcpu16_t *computer) 
{
	unsigned char cycles = 0;

	// Get the next instruction
	DCPU16_WORD w = computer->ram[computer->registers[DCPU16_INDEX_REG_PC]];
	char opcode = w & 0xF;

	computer->registers[DCPU16_INDEX_REG_PC]++;

	// Find out if it is a non basic or basic instruction
	if(opcode == DCPU16_OPCODE_NON_BASIC) {
		// Non-basic instruction
		char o = (w >> 4) & 0x3F;
		char a = (w >> 10) & 0x3F;

		// Temporary storage for embedded literal values
		DCPU16_WORD a_literal_tmp;

		// Get pointer to A
		DCPU16_WORD *a_word;
		cycles += dcpu16_get_pointer(computer, a, &a_literal_tmp, &a_word);

		switch(o) {
		case DCPU16_NON_BASIC_OPCODE_RESERVED_0:
				
			return cycles;
		case DCPU16_NON_BASIC_OPCODE_JSR_A:
			dcpu16_decrease_sp(computer);
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
		DCPU16_WORD *b_word;
		DCPU16_WORD *a_word;
		cycles += dcpu16_get_pointer(computer, a, &a_literal_tmp, &a_word);
		cycles += dcpu16_get_pointer(computer, b, &b_literal_tmp, &b_word);

		// Give up if illegal instruction detected (trying set a literal value)
		char a_literal = dcpu16_is_literal(a);

		if(a_literal && opcode >= DCPU16_OPCODE_SET && opcode <= DCPU16_OPCODE_XOR) 
			return 0; // TODO: find out if it is legal to return 0 cycles in this case.

		switch(opcode) {
		case DCPU16_OPCODE_SET:
			cycles += 1;
			dcpu16_set(computer, a_word, dcpu16_get(computer, b_word));

			return cycles;
		case DCPU16_OPCODE_ADD:
			cycles += 2;
	
			if((int) dcpu16_get(computer, a_word) + (int) dcpu16_get(computer, b_word) > 0xFFFF) {
				computer->registers[DCPU16_INDEX_REG_O] = 1;
			} else {
				computer->registers[DCPU16_INDEX_REG_O] = 0;
			}

			dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) + dcpu16_get(computer, b_word));

			return cycles;
		case DCPU16_OPCODE_SUB:
			cycles += 2;

			if((int) dcpu16_get(computer, a_word) - (int) dcpu16_get(computer, b_word) < 0) {
				computer->registers[DCPU16_INDEX_REG_O] = 0xFFFF;
			} else {
				computer->registers[DCPU16_INDEX_REG_O] = 0;
			}

			dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) - dcpu16_get(computer, b_word));
	
			return cycles;
		case DCPU16_OPCODE_MUL:
			cycles += 2;

			computer->registers[DCPU16_INDEX_REG_O] = ((dcpu16_get(computer, a_word) *dcpu16_get(computer, b_word)) >> 16) & 0xFFFF;
			dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) *dcpu16_get(computer, b_word));

			return cycles;
		case DCPU16_OPCODE_DIV:
			cycles += 3;

			if(dcpu16_get(computer, b_word) == 0) {
				computer->registers[DCPU16_INDEX_REG_A] = 0;
				computer->registers[DCPU16_INDEX_REG_O] = 0;
			} else {
				computer->registers[DCPU16_INDEX_REG_O] = ((dcpu16_get(computer, a_word) << 16) / dcpu16_get(computer, b_word)) & 0xFFFF;
				dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) / dcpu16_get(computer, b_word));
			}

			return cycles;
		case DCPU16_OPCODE_MOD:
			cycles += 3;

			if(dcpu16_get(computer, b_word) == 0) {
				computer->registers[DCPU16_INDEX_REG_A] = 0;
			} else {
				dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) % dcpu16_get(computer, b_word));
			}

			return cycles;
		case DCPU16_OPCODE_SHL:
			cycles += 2;

			computer->registers[DCPU16_INDEX_REG_O] = ((dcpu16_get(computer, a_word) << dcpu16_get(computer, b_word)) >> 16) & 0xFFFF;
			dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) << dcpu16_get(computer, b_word));

			return cycles;
		case DCPU16_OPCODE_SHR:
			cycles += 2;

			computer->registers[DCPU16_INDEX_REG_O] = ((dcpu16_get(computer, a_word) << 16) >> dcpu16_get(computer, b_word)) & 0xFFFF;
			dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) >> dcpu16_get(computer, b_word));

			return cycles;
		case DCPU16_OPCODE_AND:
			cycles += 1;

			dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) & dcpu16_get(computer, b_word));

			return cycles;
		case DCPU16_OPCODE_BOR:
			cycles += 1;

			dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) | dcpu16_get(computer, b_word));

			return cycles;
		case DCPU16_OPCODE_XOR:
			cycles += 1;

			dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) ^ dcpu16_get(computer, b_word));

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


	// Call the PC callback
	dcpu16_pc_callback(computer);

	return cycles;
}

/* Displays the contents of the registers. */
void dcpu16_print_registers(dcpu16_t *computer)
{
	printf("--------------------------------------------------------------\n");
	printf("a:%x b:%x c:%x x:%x y:%x z:%x i:%x j:%x pc:%x sp:%x o:%x\n", 
		computer->registers[DCPU16_INDEX_REG_A], computer->registers[DCPU16_INDEX_REG_B], computer->registers[DCPU16_INDEX_REG_C],
		computer->registers[DCPU16_INDEX_REG_X], computer->registers[DCPU16_INDEX_REG_Y], computer->registers[DCPU16_INDEX_REG_Z],
		computer->registers[DCPU16_INDEX_REG_I], computer->registers[DCPU16_INDEX_REG_J], computer->registers[DCPU16_INDEX_REG_PC],
		computer->registers[DCPU16_INDEX_REG_SP], computer->registers[DCPU16_INDEX_REG_O]);
	printf("--------------------------------------------------------------\n");
}

/* Prints the contents of the RAM. */
void dcpu16_dump_ram(dcpu16_t *computer, DCPU16_WORD start, DCPU16_WORD end)
{
	// Check if end is out ouf bounds
	if(end >= DCPU16_RAM_SIZE - 8)
		end = DCPU16_RAM_SIZE - 9;

	// Align start and end addresses to multiples of 8
	if(start % 8 != 0) {
		int tmp = start / 8;
		start = tmp *8;
	}

	if(end % 8 != 0) {
		int tmp = end / 8;
		end = (tmp + 1) *8;
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

/* Initializes the emulator (clears the registers and RAM) */
void dcpu16_init(dcpu16_t *computer)
{
	memset(computer, 0 , sizeof(*computer));
}

/* Loads a program into the RAM, returns 1 on success.
   If binary equals 1 the file is opened as a binary file and it expects the 16-bit integers to be stored in little endian order. */
int dcpu16_load_ram(dcpu16_t *computer, char *file, char binary)
{
	FILE *f;
	DCPU16_WORD *ram_p = computer->ram;

	if(binary) {
		// Open the file for binary reading
		f = fopen(file, "rb");
		if(!f) {
			printf("Couldn't open RAM file.\n");
			return 0;
		}

		// Read the 16-bit integers
		while(!feof(f)) {
			if(ram_p > computer->ram + DCPU16_RAM_SIZE) {
				printf("RAM file is too large.\n");
				fclose(f);
				return 0;
			}

			DCPU16_WORD w;
			w = fgetc(f);
			w = w | (fgetc(f) << 8);
			*ram_p = w;
			ram_p++;
		}
	} else {
		// Open the file for reading
		f = fopen(file, "r");
		if(!f) {
			printf("Couldn't open RAM file.\n");
			return 0;
		}

		// Read the 16-bit integers
		while(!feof(f)) {
			if(ram_p > computer->ram + DCPU16_RAM_SIZE) {
				printf("RAM file is too large.\n");
				fclose(f);
				return 0;
			}

			DCPU16_WORD w;
			fscanf(f, "%hx", &w);
			*ram_p = w;
			ram_p++;
		}

	}

	fclose(f);

	DCPU16_WORD words_loaded = ram_p - computer->ram;
	printf("Loaded %d words into RAM\n", (int)words_loaded);
	
	return 1;
}



void dcpu16_enter_ram(dcpu16_t *computer)
{
	printf("ENTER RAM CONTENT MANUALLY\n");
	printf("Expects lower case letters, hexadecimal 16-bit integers.\n\n");
	printf("Type ENTER without entering a number when you are done."
		"The rest of the RAM will be filled with zeroes.\n\n");

	DCPU16_WORD word;
	DCPU16_WORD *ram_p = computer->ram;

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
	printf("DCPU16 emulator now running in debug mode\n"
		"\tType 's' to execute the next instruction\n"
		"\tType 'r' to print the contents of the registers\n"
		"\tType 'd' to display what's in the RAM\n"
		"\tType 'q' to quit\n\n");

	char i = 0;

	while(i != 'q') {
		// Get new user input
		i = getchar();

		if(i == 's') {
			// Step
			int pc_before = computer->registers[DCPU16_INDEX_REG_PC];
			int cycles = dcpu16_step(computer);
			printf("pc: %.4x | instruction: %.4x | cycles: %d | pc afterwards: %.4x\t\n\n",
				pc_before, computer->ram[pc_before], cycles, computer->registers[DCPU16_INDEX_REG_PC]);
		} else if(i == 'r') {
			// Display the contents of the registers
			dcpu16_print_registers(computer);
			putchar('\n');

		} else if(i == 'd') {
			// RAM dump
			DCPU16_WORD d_start;
			DCPU16_WORD d_end;

			printf("\nRAM dump start address (hex): 0x");
			scanf("%hx", &d_start);
			printf("RAM dump end address (hex): 0x");
			scanf("%hx", &d_end);

			dcpu16_dump_ram(computer, d_start, d_end);
		}
	}
}

void dcpu16_profiler_step(dcpu16_t *computer)
{
	computer->profiling.instruction_count++;
	
	if ((computer->profiling.instruction_count % 1000) == 0) {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		double now = (double)tv.tv_sec + ((double)tv.tv_usec *0.000001);
		
		// If sampling was just enabled, then show first sample sample_frequency seconds from now
		if (computer->profiling.sample_time == 0.0)
			computer->profiling.sample_time = now;
		
		double sample_elapsed = now - computer->profiling.sample_time;
		if (sample_elapsed >= computer->profiling.sample_frequency) {
			// Time since last sample was taken
			double instructions_per_second = (double)computer->profiling.instruction_count / sample_elapsed;
			
			printf("[ PROFILE ]\nSample Duration: %.3lf\nInstructions: %u\nMHz: %.2lf\n-----------\n",
				   sample_elapsed, computer->profiling.instruction_count, (instructions_per_second / 1000000.0));
			
			// Reset instruction count
			computer->profiling.instruction_count = 0;
			
			// Remember when this sample was taken (for next time)
			computer->profiling.sample_time = now;
		}
	}
}

void dcpu16_run(dcpu16_t *computer)
{
	printf("DCPU16 emulator now running\n");

	/* Timo Fixme: This infinite loop detection is quite slow and interferes with performance profiling.  Turned it off for now. */
	/* while(!(computer->ram[computer->registers[DCPU16_INDEX_REG_PC]] == (((0x20 + computer->registers[DCPU16_INDEX_REG_PC]) << 10) | 1) ||
	 computer->ram[computer->registers[DCPU16_INDEX_REG_PC]] == 0x7DC1 && 
	 computer->ram[(DCPU16_WORD)(computer->registers[DCPU16_INDEX_REG_PC] + 1)] == computer->registers[DCPU16_INDEX_REG_PC])) */
	while(1) {
		dcpu16_step(computer);

		// Profiling
		if (computer->profiling.enabled != 0)
			dcpu16_profiler_step(computer);
	}
	
	printf("Infinite loop detected (reached end of code?)\n");
	printf("Emulator halted\n\n");

	// Print the state of the registers
	printf("STATE:\n");
	dcpu16_print_registers(computer);

	// Let the user explore the state
	char i = 0;

	printf("\nYou can now explore the state of the machine\n"
		"\tType 'r' to print the contents of the registers\n"
		"\tType 'd' to display what's in the RAM\n"
	       	"\tType 'q' to quit\n\n");

	while(i != 'q') {
		i = getchar();

		if(i == 'r') {
			// Display the contents of the registers
			dcpu16_print_registers(computer);
			putchar('\n');

		} else if(i == 'd') {
			// RAM dump
			DCPU16_WORD d_start;
			DCPU16_WORD d_end;

			printf("\nRAM dump start address: 0x");
			scanf("%hx", &d_start);
			printf("RAM dump end address: 0x");
			scanf("%hx", &d_end);

			dcpu16_dump_ram(computer, d_start, d_end);
		}
	}
}

int main(int argc, char *argv[]) 
{
	dcpu16_t computerOnTheStack;
	dcpu16_t *computer = &computerOnTheStack;
	dcpu16_init(computer);

	// No callback functions
	computerOnTheStack.callback.unmapped_ram_changed = 0;
	computerOnTheStack.callback.register_changed = 0;

	// Command line arguments
	char *ram_file 	= 0;
	char binary_ram_file 	= 0;
	char debug_mode 	= 0;
	char enable_profiling = 0;
	
	// Parse the arguments
	for(int c = 1; c < argc; c++) {
		if(strcmp(argv[c], "-d") == 0) {
			debug_mode = 1;
		} else if(strcmp(argv[c], "-b") == 0) {
			binary_ram_file = 1;
		} else if(strcmp(argv[c], "-p") == 0) {
			enable_profiling = 1;
		} else {
			ram_file = argv[c];
		}
	}


	// Load the RAM file or let the user enter program manually
	if(ram_file) {
		if(binary_ram_file) {
			if(!dcpu16_load_ram(computer, ram_file, 1)) {
				return 0;
			}
		} else {
			if(!dcpu16_load_ram(computer, ram_file, 0)) {
				return 0;
			}
		}
		
	} else {
		dcpu16_enter_ram(computer);
	}

	if (enable_profiling) {
		// Enable profiling
		computer->profiling.enabled = 1;
		
		// Show sample data every 1.0 seconds
		computer->profiling.sample_frequency = 1.0;
	}

	// Start the emulator
	if(debug_mode)
		dcpu16_run_debug(computer);
	else
		dcpu16_run(computer);
	
	return 0;
}
