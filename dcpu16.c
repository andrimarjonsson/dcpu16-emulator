#include <stdio.h>
#include <string.h>
#define __need_struct_timeval 1
#include <sys/time.h>
#include "dcpu16.h"

/* Installs the device and returns a non-negative value on success. The returned value is the index/slot where the device was installed. */
int dcpu16_install_device(dcpu16_t *computer, dcpu16_device_t *device)
{
	for(int slot = 0; slot < DCPU16_DEVICE_SLOTS; slot++) {
		if(!computer->devices[slot]) {
			computer->devices[slot] = device;
			return slot;
		}
	}

	return -1;
}

/* Calls the device's release function and then removes it from the computer. */
void dcpu16_release_device(dcpu16_t *computer, int slot)
{
	if(computer->devices[slot]) {
		if(computer->devices[slot]->release)
			computer->devices[slot]->release();

		computer->devices[slot] = 0;
	}
}

/* Finds the device which is mapped to the specified memory address.
   Returns a pointer to the dcpu16_device_t structure or 0 if the address is unmapped. */
static dcpu16_device_t * dcpu16_mapped_device(dcpu16_t *computer, DCPU16_WORD address) 
{
	for(int slot = 0; slot < DCPU16_DEVICE_SLOTS; slot++) {
		if(computer->devices[slot]) {
			if(computer->devices[slot]->ram_start_address <= address &&
			   computer->devices[slot]->ram_end_address >= address)
				return computer->devices[slot];
		}
	}

	return 0;
}

/* Must be used when setting the value of ANY register or any RAM of the emulated computer. */
static inline void dcpu16_set(dcpu16_t *computer, DCPU16_WORD *where, DCPU16_WORD value)
{
	if(where >= computer->ram && where < computer->ram + DCPU16_RAM_SIZE) {	// RAM
		// Calculate the RAM address
		DCPU16_WORD ram_address = where - computer->ram;

		// Check for hardware mapped RAM
		dcpu16_device_t * dev = dcpu16_mapped_device(computer, ram_address);
		if(dev) {
			dev->write(ram_address - dev->ram_start_address, value);
		} else {
			// Call the callback function if address was not hardware mapped
			if(computer->callback.unmapped_ram_changed)
				computer->callback.unmapped_ram_changed(ram_address, value);

			// Write to RAM
			*where = value;
		}
	} else if(where >= computer->registers && where < computer->registers + DCPU16_REGISTER_COUNT) {	// Register
		// Call the callback function
		if(computer->callback.register_changed)
			computer->callback.register_changed(where - computer->registers, value);

		// Set the register value
		*where = value;
	}
}

/* Must be used when getting the value of ANY register or any RAM of the emulated computer. */
static inline DCPU16_WORD dcpu16_get(dcpu16_t *computer, DCPU16_WORD *where)
{
	if(where >= computer->ram && where < computer->ram + DCPU16_RAM_SIZE) {	// RAM
		// Calculate the RAM address
		DCPU16_WORD ram_address = where - computer->ram;

		// Check for hardware mapped RAM
		dcpu16_device_t * dev = dcpu16_mapped_device(computer, ram_address);
		if(dev) {
			return dev->read(ram_address - dev->ram_start_address);
		} else {
			// Read from RAM
			return *where;
		}
	} else if(where >= computer->registers && where < computer->registers + DCPU16_REGISTER_COUNT) {	// Register
		return *where;
	} else {
		return *where;		// Constant value
	}
}

/* Call this when the callback for PC changed should be called. */
static inline void dcpu16_pc_callback(dcpu16_t *computer)
{	
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
static unsigned char dcpu16_get_pointer(dcpu16_t *computer, unsigned char where, DCPU16_WORD *tmp_storage, DCPU16_WORD **retval)
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

/* Returns true if v is a literal (v is expected to be a 6-bit AB value). */
static inline char dcpu16_is_literal(char v)
{
	if(v == DCPU16_AB_VALUE_WORD || (v >= 0x20 && v <= 0x3F))
		return 1;

	return 0;
}

/* Skips the next instruction (advances PC). */
static void dcpu16_skip_next_instruction(dcpu16_t *computer)
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

			break;
		case DCPU16_OPCODE_ADD:
			cycles += 2;
	
			if((int) dcpu16_get(computer, a_word) + (int) dcpu16_get(computer, b_word) > 0xFFFF) {
				computer->registers[DCPU16_INDEX_REG_O] = 1;
			} else {
				computer->registers[DCPU16_INDEX_REG_O] = 0;
			}

			dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) + dcpu16_get(computer, b_word));

			break;
		case DCPU16_OPCODE_SUB:
			cycles += 2;

			if((int) dcpu16_get(computer, a_word) - (int) dcpu16_get(computer, b_word) < 0) {
				computer->registers[DCPU16_INDEX_REG_O] = 0xFFFF;
			} else {
				computer->registers[DCPU16_INDEX_REG_O] = 0;
			}

			dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) - dcpu16_get(computer, b_word));
	
			break;
		case DCPU16_OPCODE_MUL:
			cycles += 2;

			computer->registers[DCPU16_INDEX_REG_O] = ((dcpu16_get(computer, a_word) *dcpu16_get(computer, b_word)) >> 16) & 0xFFFF;
			dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) *dcpu16_get(computer, b_word));

			break;
		case DCPU16_OPCODE_DIV:
			cycles += 3;

			if(dcpu16_get(computer, b_word) == 0) {
				computer->registers[DCPU16_INDEX_REG_A] = 0;
				computer->registers[DCPU16_INDEX_REG_O] = 0;
			} else {
				computer->registers[DCPU16_INDEX_REG_O] = ((dcpu16_get(computer, a_word) << 16) / dcpu16_get(computer, b_word)) & 0xFFFF;
				dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) / dcpu16_get(computer, b_word));
			}

			break;
		case DCPU16_OPCODE_MOD:
			cycles += 3;

			if(dcpu16_get(computer, b_word) == 0) {
				computer->registers[DCPU16_INDEX_REG_A] = 0;
			} else {
				dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) % dcpu16_get(computer, b_word));
			}

			break;
		case DCPU16_OPCODE_SHL:
			cycles += 2;

			computer->registers[DCPU16_INDEX_REG_O] = ((dcpu16_get(computer, a_word) << dcpu16_get(computer, b_word)) >> 16) & 0xFFFF;
			dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) << dcpu16_get(computer, b_word));

			break;
		case DCPU16_OPCODE_SHR:
			cycles += 2;

			computer->registers[DCPU16_INDEX_REG_O] = ((dcpu16_get(computer, a_word) << 16) >> dcpu16_get(computer, b_word)) & 0xFFFF;
			dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) >> dcpu16_get(computer, b_word));

			break;
		case DCPU16_OPCODE_AND:
			cycles += 1;

			dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) & dcpu16_get(computer, b_word));

			break;
		case DCPU16_OPCODE_BOR:
			cycles += 1;

			dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) | dcpu16_get(computer, b_word));

			break;
		case DCPU16_OPCODE_XOR:
			cycles += 1;

			dcpu16_set(computer, a_word, dcpu16_get(computer, a_word) ^ dcpu16_get(computer, b_word));

			break;
		case DCPU16_OPCODE_IFE:
			cycles += 2;
			
			if(dcpu16_get(computer, a_word) != dcpu16_get(computer, b_word))
			{
				dcpu16_skip_next_instruction(computer);
				cycles++;
			}

			break;
		case DCPU16_OPCODE_IFN:
			cycles += 2;

			if(dcpu16_get(computer, a_word) == dcpu16_get(computer, b_word))
			{
				dcpu16_skip_next_instruction(computer);
				cycles++;
			}

			break;
		case DCPU16_OPCODE_IFG:
			cycles += 2;

			if(dcpu16_get(computer, a_word) <= dcpu16_get(computer, b_word))
			{
				dcpu16_skip_next_instruction(computer);
				cycles++;
			}

			break;
		case DCPU16_OPCODE_IFB:
			cycles += 2;	

			if(dcpu16_get(computer, a_word) & dcpu16_get(computer, b_word) == 0)
			{
				dcpu16_skip_next_instruction(computer);
				cycles++;
			}

			break;
		};
	}


	// Call the PC callback
	dcpu16_pc_callback(computer);

	return cycles;
}

/* Displays the contents of the registers. */
void dcpu16_print_registers(dcpu16_t *computer)
{
	PRINTF("--------------------------------------------------------------\n");
	PRINTF("a:%x b:%x c:%x x:%x y:%x z:%x i:%x j:%x pc:%x sp:%x o:%x\n", 
		computer->registers[DCPU16_INDEX_REG_A], computer->registers[DCPU16_INDEX_REG_B], computer->registers[DCPU16_INDEX_REG_C],
		computer->registers[DCPU16_INDEX_REG_X], computer->registers[DCPU16_INDEX_REG_Y], computer->registers[DCPU16_INDEX_REG_Z],
		computer->registers[DCPU16_INDEX_REG_I], computer->registers[DCPU16_INDEX_REG_J], computer->registers[DCPU16_INDEX_REG_PC],
		computer->registers[DCPU16_INDEX_REG_SP], computer->registers[DCPU16_INDEX_REG_O]);
	PRINTF("--------------------------------------------------------------\n");
}

/* Prints the contents of the RAM. */
void dcpu16_dump_ram(dcpu16_t *computer, DCPU16_WORD start, DCPU16_WORD end)
{
	// Align start and end addresses to multiples of 8
	if(start % 8 != 0) {
		int tmp = start / 8;
		start = tmp *8;
	}

	if(end % 8 != 0) {
		int tmp = end / 8;
		end = (tmp + 1) *8;
	}

	// Let the printing begin
	PRINTF("\nRAM DUMP\n");

	for(; start <= end; start+=8) {
		PRINTF("%.4x:", start);

		for(int i = 0; i < 8; i++) {
			DCPU16_WORD w = computer->ram[start + i];
			PRINTF("%.4x ", w);
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

/* Loads a program into the RAM, returns true on success.
   If binary is false the file is opened as a binary file and it expects the 16-bit integers to be stored in little endian order. */
int dcpu16_load_ram(dcpu16_t *computer, const char *file, char binary)
{
	FILE *f;
	DCPU16_WORD * ram_p = computer->ram;

	// Open the file for reading
	if(binary)
		f = fopen(file, "rb");
	else
		f = fopen(file, "r");

	// Make sure it's open
	if(!f)
		return 0;

	// Read the contents into RAM
	while(!feof(f)) {
		// Make sure we can fit more in RAM
		if(ram_p > computer->ram + DCPU16_RAM_SIZE) {
			fclose(f);
			return 0;
		}

		// Read and put in RAM
		DCPU16_WORD w;

		if(binary) {
			w = fgetc(f);
			w = w | (fgetc(f) << 8);
		} else {
			fscanf(f, "%hx", &w);
		}

		*ram_p = w;
		ram_p++;
	}

	fclose(f);

	// Calculate the number of words loaded
	DCPU16_WORD words_loaded = ram_p - computer->ram;
	PRINTF("Loaded %d words into RAM\n", (int)words_loaded);
	
	return 1;
}


/* Reads one character using getchar() and does the following depending on the character read:
   r - prints the contents of the registers
   d - ram dump
   Return value is 0 if the read character has been handled by this function, otherwise the character is returned. */
static char dcpu16_explore_state(dcpu16_t *computer)
{
	char c = getchar();

	if(c == 'r') {
		dcpu16_print_registers(computer);
	} else if(c == 'd') {
		DCPU16_WORD d_start;
		DCPU16_WORD d_end;

		PRINTF("\nRAM dump start address (hex): 0x");
		scanf("%hx", &d_start);
		PRINTF("RAM dump end address (hex): 0x");
		scanf("%hx", &d_end);

		dcpu16_dump_ram(computer, d_start, d_end);
	} else {
		return c;
	}

	return 0;
}


static void dcpu16_run_debug(dcpu16_t *computer)
{
	PRINTF("DCPU16 emulator now running in debug mode\n"
		"\tType 's' to execute the next instruction\n"
		"\tType 'r' to print the contents of the registers\n"
		"\tType 'd' to display what's in the RAM\n"
		"\tType 'q' to quit\n\n");

	char c = 0;
	while(c != 'q') {
		c = dcpu16_explore_state(computer);

		if(c == 's') {
			// Step
			int pc_before = computer->registers[DCPU16_INDEX_REG_PC];
			int cycles = dcpu16_step(computer);
			PRINTF("pc: %.4x | instruction: %.4x | cycles: %d | pc afterwards: %.4x\t\n\n",
				pc_before, computer->ram[pc_before], cycles, computer->registers[DCPU16_INDEX_REG_PC]);
		}
	}
}

static void dcpu16_profiler_step(dcpu16_t *computer)
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
			
			PRINTF("[ PROFILE ]\nSample Duration: %.3lf\nInstructions: %u\nMHz: %.2lf\n-----------\n",
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
	PRINTF("DCPU16 emulator now running\n");

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

	PRINTF("Emulator halted\n\n");

	// Let the user explore the state
	PRINTF("\nYou can now explore the state of the machine\n"
		"\tType 'r' to print the contents of the registers\n"
		"\tType 'd' to display what's in the RAM\n"
	       	"\tType 'q' to quit\n\n");

	char c = 0;
	while(c != 'q') {
		c = dcpu16_explore_state(computer);
	}
}

int main(int argc, char *argv[]) 
{
	dcpu16_t computerOnTheStack;
	dcpu16_t *computer = &computerOnTheStack;
	dcpu16_init(computer);

	// Command line arguments
	char *ram_file 	= 0;
	char binary_ram_file 	= 0;
	char debug_mode 	= 0;
	char enable_profiling 	= 0;
	
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

	// Load RAM file
	if(ram_file) {
		if(!dcpu16_load_ram(computer, ram_file, binary_ram_file)) {
			PRINTF("Couldn't load RAM file (too large or bad file).\n");
			return 0;
		}
	} else {
		PRINTF("No RAM file specified.\n");
		return 0;
	}

	// Profiling
	if(enable_profiling) {
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
