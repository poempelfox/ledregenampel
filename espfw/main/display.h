
/* Display helper functions (usually not hardware specific) */

#ifndef _DISPLAY_H_
#define _DISPLAY_H_

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
 * As we have variable width fonts, this depends on the text.
 * If nc >= 0, then this will only take (at most) the first nc
 * characters into account. */
int di_calctextwidth(struct font * fo, uint8_t * txt, int nc);

/* Tiny helper to calculate the x position where a text needs to
 * be put to appear centered between x1 and x2. */
int di_calctextcenter(struct font * fo, int x1, int x2, uint8_t * txt);

/* Helper to handle Umlauts and other special chars:
 * Replaces known (!) UTF8 special chars in the string with the
 * nonstandard encoding used in our fonts.
 * Modifies the string inplace! */
void di_convertencoding(uint8_t * s);

/* Update the display from the 2 messages.
 * This will first merge the 2 messages together, then split them
 * into up to 3 lines, then display them fullscreen on the display. */
void di_updateoledwith2msgs(struct di_dispbuf * db, uint8_t * m1, uint8_t * m2);

#endif /* _DISPLAY_H_ */

