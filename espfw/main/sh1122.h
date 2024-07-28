
/* Talking to a SH1122 based I2C OLED display */

#ifndef _SH1122_H_
#define _SH1122_H_

#include "display.h" /* required for definition of struct di_dispbuf */

/* Initialize the SH1122 based display module */
void sh1122_init(void);

/* Configures the invert mode of the display. */
void sh1122_setinvertmode(uint8_t invert);

/* Display a dispbuf on that display */
void sh1122_display(struct di_dispbuf * db);

#endif /* _SH1122_H_ */

