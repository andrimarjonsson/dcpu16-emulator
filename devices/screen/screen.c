#include "screen.h"

screen_t * screen_create_device(dcpu16_device_t * dev)
{
	dev->ram_start_address = SCREEN_RAM_START_ADDRESS;
	dev->ram_end_address = SCREEN_RAM_END_ADDRESS;

	dev->write = screen_write;
	dev->read = screen_read;

	dev->struct_ptr = malloc(sizeof(screen_t));

	return dev.struct_ptr;
}

void screen_release_device(dcpu16_device_t * dev)
{
	free(dev.struct_ptr);
}

static void screen_write(dcpu16_device_t * dev, DCPU16_WORD address, DCPU16_WORD value)
{
	dev->screen_buffer[address] = value;

	if(dev->screen_changed_callback)
		dev->screen_changed_callback(address / SCREEN_COLUMNS, address % SCREEN_COLUMNS, value);
}

static DCPU16_WORD screen_read(dcpu16_device_t * dev, DCPU16_WORD address)
{
	return dev->screen_buffer[address];
}

