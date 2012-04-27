#include "screen.h"

/* Sets the target dcpu16_device_t structure to represent the screen. */
void screen_device(dcpu16_device_t * dev)
{
	dev->ram_start_address = 0;
	dev->ram_end_address = 0;

	dev->initialize = screen_initialize;
	dev->release = screen_release;
	dev->write = screen_write;
	dev->read = screen_read;
}

static void screen_initialize()
{

}

static void screen_release()
{

}

static void screen_write(DCPU16_WORD address, DCPU16_WORD value)
{

}

static DCPU16_WORD screen_read(DCPU16_WORD address)
{

}


