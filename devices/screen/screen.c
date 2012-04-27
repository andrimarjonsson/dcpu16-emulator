#include "screen.h"

static void (* screen_changed_callback)(char x, char y, DCPU16_WORD value) = 0;
static DCPU16_WORD screen_buffer[SCREEN_COLUMNS * SCREEN_ROWS];

void screen_create_device(dcpu16_device_t * dev)
{
	dev->ram_start_address = SCREEN_RAM_START_ADDRESS;
	dev->ram_end_address = SCREEN_RAM_END_ADDRESS;

	dev->initialize = screen_initialize;
	dev->release = screen_release;
	dev->write = screen_write;
	dev->read = screen_read;
}

void screen_set_callback(void (* callback)(char x, char y, DCPU16_WORD value))
{
	screen_changed_callback = callback;
}

static void screen_initialize()
{
	memset(screen_buffer, 0, sizeof(screen_buffer));
}

static void screen_release()
{
}

static void screen_write(DCPU16_WORD address, DCPU16_WORD value)
{
	screen_buffer[address] = value;

	if(screen_changed_callback)
		screen_changed_callback(address / SCREEN_COLUMNS, address % SCREEN_COLUMNS, value);
}

static DCPU16_WORD screen_read(DCPU16_WORD address)
{
	return screen_buffer[address];
}

