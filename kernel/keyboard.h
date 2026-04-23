#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include "idt.h"

#define KEYBOARD_DATA_PORT 0x60   /* read scancode from here */
#define KEYBOARD_STATUS_PORT 0x64 /* bit 0 = output buffer full */

void keyboard_init(void);
void keyboard_handler(registers_t *regs);

#endif
