#include <stdint.h>

/* start byte 0x7F
 * escape for 0x7F and 0x7E
 * 0 = 0x7E, 1 = 0x7F */
#define SERIAL_ESCAPE_BYTE	0x7E
#define SERIAL_START	0x7F

#define BAUDRATE        115200
