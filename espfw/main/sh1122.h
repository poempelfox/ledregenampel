
/* Talking to a SH1122 based I2C OLED display */

#ifndef _SH1122_H_
#define _SH1122_H_

#include "fonts/font.h"

/* This is a display buffer that holds all screen output, for
 * drawing onto it and later sending it to the display hardware
 * for displaying it.
 */
struct di_dispbuf {
  uint16_t sizex;
  uint16_t sizey;
  uint8_t * cont; /* Simply one byte per pixel, even if the SH1122 only has 4 bits per pixel. */
};

/* Initialize the SH1122 based display module */
void sh1122_init(void);

/* Configures the invert mode of the display. */
void sh1122_setinvertmode(uint8_t invert);

/* Display a dispbuf on that display */
void sh1122_display(struct di_dispbuf * db);

/* Initialize a new display buffer for drawing onto. */
struct di_dispbuf * di_newdispbuf(void);

/* Free a previously allocated display buffer */
void di_freedispbuf(struct di_dispbuf * db);

/* set a pixel */
void di_setpixel(struct di_dispbuf * db, int x, int y, uint8_t co);

/* get a pixel */
uint8_t di_getpixel(struct di_dispbuf * db, int x, int y);

/* Draw a rectangle. Use borderwidth <= 0 for a fully filled rectangle. */
void di_drawrect(struct di_dispbuf * db, int x1, int y1, int x2, int y2,
                 int borderwidth, uint8_t co);

/* invert all pixels. */
void di_invertall(struct di_dispbuf * db);

/* Embedded fonts. */
extern struct font font_FreeSansBold21pt;

/* Text functions */
/* Be aware that the y position is the BOTTOM of
 * the font, not the top as one might expect. */
void di_drawchar(struct di_dispbuf * db,
                 int x, int y, struct font * fo,
                 uint8_t co,
                 uint8_t ch);

void di_drawtext(struct di_dispbuf * db,
                 int x, int y, struct font * fo,
                 uint8_t co,
                 uint8_t * txt);

/* Calculate the width in pixels of a piece of text.
 * As we have variable width fonts, this depends on the text. */
int di_calctextwidth(struct font * fo, uint8_t * txt);

/* Tiny helper to calculate the x position where a text needs to
 * be put to appear centered between x1 and x2. */
int di_calctextcenter(struct font * fo, int x1, int x2, uint8_t * txt);

#endif /* _SH1122_H_ */

