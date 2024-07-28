/* Talking to a SH1122 based I2C OLED display.
 * This is a lot of copy+pasta from foxesptemp, which supports an entirely
 * different controller chip, that nonetheless uses a surprisingly
 * similiar protocol.
 * Also included in here are the basic display functions from display.c
 * because I'm lazy. */

#include <stdlib.h>
#include <string.h>
#include <driver/i2c.h>
#include <esp_log.h>
#include "sh1122.h"
#include "sdkconfig.h"
#include "settings.h"

#define SH1122BASEADDR 0x3c

#define I2C_MASTER_TIMEOUT_MS 1000  /* Timeout for I2C communication */


/* A very short summary of the protocol the display uses:
 * After addressing the display, there is always a 'control' byte, which
 * mainly determines whether the following byte(s) are a 'command' or 'data',
 * and has 6 unused bits. */
#define CONTROL_CONT 0x80  /* "Continuation" bit. 0 means only data follows. */
#define CONTROL_NOCO 0x00  /* "Continuation" bit. 0 means only data follows. */
#define CONTROL_DATA 0x40  /* "command or data" - bit set means data follows, */
#define CONTROL_CMD  0x00  /*                     otherwise it is a command. */

/* Send a 1 byte command */
static void sh1122_sendcommand1(uint8_t cmd)
{
    uint8_t tosend[2];
    tosend[0] = CONTROL_NOCO | CONTROL_CMD;
    tosend[1] = cmd;
    i2c_master_write_to_device(0, SH1122BASEADDR, tosend, 2,
                               I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

/* Send a 2 byte command */
static void sh1122_sendcommand2(uint8_t cmd1, uint8_t cmd2)
{
    uint8_t tosend[3];
    tosend[0] = CONTROL_NOCO | CONTROL_CMD;
    tosend[1] = cmd1;
    tosend[2] = cmd2;
    i2c_master_write_to_device(0, SH1122BASEADDR, tosend, 3,
                               I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

void sh1122_init(void)
{
    /* The initialization sequence is taken from the U8X8 library that
     * supports this chip and was recommended by the shop that sold us
     * this display, so the values they set are probably sane.
     * After checking: ALL of these simply are the power-on defaults
     * anyways, so this whole sequence is completely redundant. Glad I
     * wasted half an hour on it. */
    sh1122_sendcommand1(0xAE);  /* Display OFF. */
    sh1122_sendcommand1(0x40);  /* Set display start line to 0 */
    sh1122_sendcommand1(0xA0);  /* Set Segment remap */
    sh1122_sendcommand1(0xC0);  /* Set Common Output Scan direction to def. */
    sh1122_sendcommand2(0x81, 0x80);  /* Set display contrast to 128/255. */
    sh1122_sendcommand2(0xA8, 0x3F);  /* Set multiplex ratio to 64 */
    sh1122_sendcommand2(0xAD, 0x81);  /* Set DC-DC setting: bultin, 0.6SF */
    sh1122_sendcommand2(0xD5, 0x50);  /* Set Display Clock Divide Ratio / Osc Freq */
    sh1122_sendcommand2(0xD3, 0x00);  /* Set Display Offset to 0 */
    sh1122_sendcommand2(0xD9, 0x22);  /* Set Discharge/Precharge period */
    sh1122_sendcommand2(0xDB, 0x35);  /* Set VCOM deselect level to 0.770 */
    sh1122_sendcommand2(0xDC, 0x35);  /* Set VSEGM level to 0.770 */
    sh1122_sendcommand1(0x30);  /* Set discharge VSL level to 0V */

    /* Now turn on the display. */
    sh1122_sendcommand1(0xA4); /* Display ON, output follows RAM contents */
    sh1122_sendcommand1(0xA6); /* Normal non-inverted display (A7 to invert) */
    sh1122_sendcommand1(0xAF); /* Display ON in normal mode */
}

void sh1122_setinvertmode(uint8_t invert)
{
  if (invert) {
    sh1122_sendcommand1(0xA7); /* display contents inverted */
  } else {
    sh1122_sendcommand1(0xA6); /* Normal non-inverted display */
  }
}

void sh1122_display(struct di_dispbuf * db)
{
    sh1122_sendcommand2(0xB0, 0); /* Set row address: 0 */
    sh1122_sendcommand1(0x00 | 0); /* set Lower 4 bits of column address: 0 */
    sh1122_sendcommand1(0x10 | 0); /* set Higher/Upper 3 bits of column address: also 0. */
    /* The display seems to have a pretty intuitive memory layout:
     * Just 256 x 64 x 4 bits in sequence. */
    uint8_t sndbuf[129]; /* Send a whole line at once. 256 pixels * 4 bit plus control byte. */
    for (int row = 0; row < 64; row++) {
      sndbuf[0] = (CONTROL_DATA | CONTROL_NOCO);
      for (int col = 0; col < 128; col++) {
        uint8_t p1 = di_getpixel(db, (col * 2) + 0, row) >> 4;
        uint8_t p2 = di_getpixel(db, (col * 2) + 1, row) >> 4;
        sndbuf[col + 1] = (p1 << 4) | p2;
      }
      i2c_master_write_to_device(0, SH1122BASEADDR, sndbuf, 129,
                                 I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    }
}

/* non-display-specific functions - setting pixels, drawing rects and text... */

/* Initialize a new display buffer for drawing onto. */
struct di_dispbuf * di_newdispbuf(void)
{
    int sizex = 256; int sizey = 64;
    struct di_dispbuf * res;
    res = calloc(1, sizeof(struct di_dispbuf));
    if (res == NULL) {
      ESP_LOGE("sh1122.c", "Failed to allocate memory for displaybuf (1). This will fail horribly.");
      return res;
    }
    res->cont = calloc(1, (sizex * sizey * 1));
    if (res->cont == NULL) {
      ESP_LOGE("sh1122.c", "Failed to allocate memory for displaybuf (2). This will fail horribly.");
      free(res);
      return NULL;
    }
    res->sizex = sizex;
    res->sizey = sizey;
    return res;
}

/* Free a previously allocated display buffer */
void di_freedispbuf(struct di_dispbuf * db)
{
    if (db == NULL) {
      return;
    }
    if (db->cont != NULL) {
      free(db->cont);
    }
    free(db);
}

/* set a pixel */
void di_setpixel(struct di_dispbuf * db, int x, int y, uint8_t co)
{
    if ((x < 0) || (y < 0)) return;
    if ((x >= db->sizex) || (y >= db->sizey)) return;
    int offset = (y * db->sizex) + x;
    db->cont[offset] = co;
}

/* get a pixel */
uint8_t di_getpixel(struct di_dispbuf * db, int x, int y)
{
    if ((x < 0) || (y < 0)) return 0;
    if ((x >= db->sizex) || (y >= db->sizey)) return 0;
    int offset = (y * db->sizex) + x;
    return db->cont[offset];
}

static void swapint(int * a, int * b)
{
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

void di_drawrect(struct di_dispbuf * db, int x1, int y1, int x2, int y2,
                 int borderwidth, uint8_t co)
{
    if (x1 > x2) { swapint(&x1, &x2); }
    if (y1 > y2) { swapint(&y1, &y2); }
    if (borderwidth <= 0) { /* fully filled rect */
      for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
          di_setpixel(db, x, y, co);
        }
      }
    } else {
      if ((borderwidth > (x2 - x1)) || (borderwidth > (y2 - y1))) {
        /* borderwidth is larger than the distance between our borders, this
         * will thus create a fully filled rectangle. If we tried to draw
         * that with our algorithm below however we would overflow our borders.
         * So instead call ourselves with a borderwidth of -1. */
        di_drawrect(db, x1, y1, x2, y2, -1, co);
        return;
      }
      for (int bd = 0; bd < borderwidth; bd++) {
        for (int y = y1; y <= y2; y++) {
          di_setpixel(db, x1 + bd, y, co);
          di_setpixel(db, x2 - bd, y, co);
        }
        for (int x = x1; x <= x2; x++) {
          di_setpixel(db, x, y1 + bd, co);
          di_setpixel(db, x, y2 - bd, co);
        }
      }
    }
}

void di_drawchar(struct di_dispbuf * db,
                 int x, int y, struct font * fo,
                 uint8_t co,
                 uint8_t ch)
{
    //ESP_LOGI("sh1122.c", "Drawing '%c' from offset %u at %d/%d", ch, fo->offsets[ch], x, y);
    if ((ch < fo->first) || (ch > fo->last)) {
      /* This char is not in our font. */
      return;
    }
    GFXglyph gly = fo->glyph[ch - fo->first];
    uint8_t * bm = fo->bitmap + gly.bitmapOffset;
    uint8_t curbyte = *bm;
    int bitsused = 0;
    for (int yp = 0; yp < gly.height; yp++) {
      for (int xp = 0; xp < gly.width; xp++) {
        if (bitsused > 7) { // previous byte used up, load next byte
          bm++;
          curbyte = *bm;
          bitsused = 0;
        }
        if (curbyte & 0x80) { // Pixel is set!
          di_setpixel(db, x + gly.xOffset + xp, y + gly.yOffset + yp, co);
        }
        curbyte <<= 1;
        bitsused++;
      }
    }
}

void di_drawtext(struct di_dispbuf * db,
                 int x, int y, struct font * fo,
                 uint8_t co,
                 uint8_t * txt)
{
    while (*txt != 0) {
      uint8_t ch = *txt;
      if ((ch < fo->first) || (ch > fo->last)) { // Character is not in font
        // instead of just skipping, we need to make clear there is something
        // missing here.
        // we use fo->last, which should always contain the beloved
        // "unknown character symbol" (U+FFFD).
        ch = fo->last;
      }
      GFXglyph gly = fo->glyph[ch - fo->first];
      //ESP_LOGI("sh1122.c", "Drawing character '%c' at %d/%d", *txt, x, y);
      di_drawchar(db, x, y, fo, co, ch);
      x += gly.xAdvance;
      txt++;
    }
}

int di_calctextwidth(struct font * fo, uint8_t * txt)
{
    int res = 0;
    while (*txt != 0) {
      uint8_t ch = *txt;
      if ((ch < fo->first) || (ch > fo->last)) { // Character is not in font
        ch = fo->last;
      }
      GFXglyph gly = fo->glyph[ch - fo->first];
      res += gly.xAdvance;
      txt++;
    }
    return res;
}

int di_calctextcenter(struct font * fo, int x1, int x2, uint8_t * txt)
{
    return x1 + ((x2 - x1 - di_calctextwidth(fo, txt) ) / 2);
}
