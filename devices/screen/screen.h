#ifndef SCREEN_H
#define SCREEN_H

#include "dcpu16.h"

#define SCREEN_COLUMNS			// INSERT VALUE HERE
#define SCREEN_ROWS			// INSERT VALUE HERE

#define SCREEN_RAM_START_ADDRESS 	// INSERT VALUE HERE
#define SCREEN_RAM_END_ADDRESS 		SCREEN_RAM_START_ADDRESS + SCREEN_COLUMNS * SCREEN_ROWS

screen_t * screen_create_device(dcpu16_device_t * dev);
void screen_release_device(dcpu16_device_t * dev);

typedef struct _screen_t 
{
	DCPU16_WORD screen_buffer[SCREEN_COLUMNS * SCREEN_ROWS];
	void (* screen_changed_callback)(char x, char y, DCPU16_WORD value);
} screen_t;

#endif
