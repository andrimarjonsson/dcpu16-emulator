#ifndef DCPU16_H
#define DCPU16_H

typedef unsigned short DCPU16_WORD;

#define DCPU16_RAM_SIZE 			0x10000

#define DCPU16_OPCODE_NON_BASIC			0x00
#define DCPU16_OPCODE_SET			0x01
#define DCPU16_OPCODE_ADD			0x02
#define DCPU16_OPCODE_SUB			0x03
#define DCPU16_OPCODE_MUL			0x04
#define DCPU16_OPCODE_DIV			0x05
#define DCPU16_OPCODE_MOD			0x06
#define DCPU16_OPCODE_SHL			0x07
#define DCPU16_OPCODE_SHR			0x08
#define DCPU16_OPCODE_AND			0x09
#define DCPU16_OPCODE_BOR			0x0A
#define DCPU16_OPCODE_XOR			0x0B
#define DCPU16_OPCODE_IFE			0x0C
#define DCPU16_OPCODE_IFN			0x0D
#define DCPU16_OPCODE_IFG			0x0E
#define DCPU16_OPCODE_IFB			0x0F

#define DCPU16_NON_BASIC_OPCODE_RESERVED_0	0x00
#define DCPU16_NON_BASIC_OPCODE_JSR_A		0x01

#define DCPU16_AB_VALUE_REG_A			0x00
#define DCPU16_AB_VALUE_REG_B			0x01
#define DCPU16_AB_VALUE_REG_C			0x02
#define DCPU16_AB_VALUE_REG_X			0x03
#define DCPU16_AB_VALUE_REG_Y			0x04
#define DCPU16_AB_VALUE_REG_Z			0x05
#define DCPU16_AB_VALUE_REG_I			0x06
#define DCPU16_AB_VALUE_REG_J			0x07
#define DCPU16_AB_VALUE_PTR_REG_A		0x08
#define DCPU16_AB_VALUE_PTR_REG_B		0x09
#define DCPU16_AB_VALUE_PTR_REG_C		0x0A
#define DCPU16_AB_VALUE_PTR_REG_X		0x0B
#define DCPU16_AB_VALUE_PTR_REG_Y		0x0C
#define DCPU16_AB_VALUE_PTR_REG_Z		0x0D
#define DCPU16_AB_VALUE_PTR_REG_I		0x0E
#define DCPU16_AB_VALUE_PTR_REG_J		0x0F
#define DCPU16_AB_VALUE_PTR_REG_A_PLUS_WORD	0x10
#define DCPU16_AB_VALUE_PTR_REG_B_PLUS_WORD	0x11
#define DCPU16_AB_VALUE_PTR_REG_C_PLUS_WORD	0x12
#define DCPU16_AB_VALUE_PTR_REG_X_PLUS_WORD	0x13
#define DCPU16_AB_VALUE_PTR_REG_Y_PLUS_WORD	0x14
#define DCPU16_AB_VALUE_PTR_REG_Z_PLUS_WORD	0x15
#define DCPU16_AB_VALUE_PTR_REG_I_PLUS_WORD	0x16
#define DCPU16_AB_VALUE_PTR_REG_J_PLUS_WORD	0x17
#define DCPU16_AB_VALUE_POP			0x18
#define DCPU16_AB_VALUE_PEEK			0x19
#define DCPU16_AB_VALUE_PUSH			0x1A
#define DCPU16_AB_VALUE_REG_SP			0x1B
#define DCPU16_AB_VALUE_REG_PC			0x1C
#define DCPU16_AB_VALUE_REG_O			0x1D
#define DCPU16_AB_VALUE_PTR_WORD		0x1E
#define DCPU16_AB_VALUE_WORD			0x1F

#define DCPU16_REGISTER_COUNT			11
#define DCPU16_INDEX_REG_A				0
#define DCPU16_INDEX_REG_B				1
#define DCPU16_INDEX_REG_C				2
#define DCPU16_INDEX_REG_X				3
#define DCPU16_INDEX_REG_Y				4
#define DCPU16_INDEX_REG_Z				5
#define DCPU16_INDEX_REG_I				6
#define DCPU16_INDEX_REG_J				7
#define DCPU16_INDEX_REG_PC				8
#define DCPU16_INDEX_REG_SP				9
#define DCPU16_INDEX_REG_O				10

typedef struct _dcpu16_t
{
	// Used for performance profiling
	struct profiling {
		unsigned char enabled;
		double sample_time;
		double sample_frequency;
		double sample_start_time;
		unsigned instruction_count;
	} profiling;

	// Pointers to callback functions
	struct callback {
		void (* register_changed)(unsigned char reg, DCPU16_WORD val);
		void (* unmapped_ram_changed)(DCPU16_WORD address, DCPU16_WORD val);
	} callback;
	
	// All registers including PC and SP
	DCPU16_WORD registers[DCPU16_REGISTER_COUNT];
	
	// RAM
	DCPU16_WORD ram[DCPU16_RAM_SIZE];
} dcpu16_t;


/* This is useful if someone wants to redirect all the console writes.
   Just define PRINTF to whatever you want before including this header file.  */
#ifndef PRINTF
	#define PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif // PRINTF

/* Declaration of "public" functions */
void dcpu16_init(dcpu16_t *computer);
int dcpu16_load_ram(dcpu16_t *computer, const char *file, char binary);
void dcpu16_run(dcpu16_t *computer);
unsigned char dcpu16_step(dcpu16_t *computer);
void dcpu16_dump_ram(dcpu16_t *computer, DCPU16_WORD start, DCPU16_WORD end);
void dcpu16_print_registers(dcpu16_t *computer);

#endif // DCPU16_H
