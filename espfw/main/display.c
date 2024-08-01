/* basic display functions (drawing of rectangles, text), usually not
 * display hardware specific. */

#include <stdlib.h>
#include <string.h>
#include <driver/i2c.h>
#include <esp_log.h>
#include "display.h"
#include "sdkconfig.h"
#include "settings.h"
#include "sh1122.h"

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

int di_calctextwidth(struct font * fo, uint8_t * txt, int nc)
{
    int res = 0; int nsofar = 0;
    if (nc == 0) { return 0; }
    while (*txt != 0) {
      uint8_t ch = *txt;
      if ((ch < fo->first) || (ch > fo->last)) { // Character is not in font
        ch = fo->last;
      }
      GFXglyph gly = fo->glyph[ch - fo->first];
      res += gly.xAdvance;
      txt++;
      nsofar++;
      if ((nc >= 0) && (nsofar >= nc)) break;
    }
    return res;
}

int di_calctextcenter(struct font * fo, int x1, int x2, uint8_t * txt)
{
    return x1 + ((x2 - x1 - di_calctextwidth(fo, txt, -1) ) / 2);
}

void di_convertencoding(uint8_t * s)
{
  uint8_t * rp = s;
  uint8_t * wp = s;
  while (*rp != 0) {
    if ((*(rp+0) == 0xC2) && (*(rp+1) == 0xB0)) { // degree sign
      *wp = 0x7F; rp += 2; wp += 1;
    } else if ((*(rp+0) == 0xC2) && (*(rp+1) == 0xB2)) { // superscript 2
      *wp = 0x80; rp += 2; wp += 1;
    } else if ((*(rp+0) == 0xC2) && (*(rp+1) == 0xB3)) { // superscript 3
      *wp = 0x81; rp += 2; wp += 1;
    } else if ((*(rp+0) == 0xC2) && (*(rp+1) == 0xB5)) { // micro
      *wp = 0x82; rp += 2; wp += 1;
    } else if ((*(rp+0) == 0xC3) && (*(rp+1) == 0x84)) { // Umlaut Ae
      *wp = 0x83; rp += 2; wp += 1;
    } else if ((*(rp+0) == 0xC3) && (*(rp+1) == 0x96)) { // Umlaut Oe
      *wp = 0x84; rp += 2; wp += 1;
    } else if ((*(rp+0) == 0xC3) && (*(rp+1) == 0x9C)) { // Umlaut Ue
      *wp = 0x85; rp += 2; wp += 1;
    } else if ((*(rp+0) == 0xC3) && (*(rp+1) == 0xA4)) { // Umlaut ae
      *wp = 0x86; rp += 2; wp += 1;
    } else if ((*(rp+0) == 0xC3) && (*(rp+1) == 0xB6)) { // Umlaut oe
      *wp = 0x87; rp += 2; wp += 1;
    } else if ((*(rp+0) == 0xC3) && (*(rp+1) == 0xBC)) { // Umlaut ue
      *wp = 0x88; rp += 2; wp += 1;
    } else if ((*(rp+0) == 0xC2) && (*(rp+1) == 0xB1)) { // plusminus
      *wp = 0x89; rp += 2; wp += 1;
    } else if ((*(rp+0) == 0xC3) && (*(rp+1) == 0x9F)) { // scharfes s (&szlig;)
      *wp = 0x8A; rp += 2; wp += 1;
    } else {
      *wp = *rp; wp++; rp++;
    }
  }
  *wp = 0;
}

void di_updateoledwith2msgs(struct di_dispbuf * db, uint8_t * m1, uint8_t * m2)
{
    uint8_t mm[220];
    strcpy(mm, m1);
    strcat(mm, " - ");
    strcat(mm, m2);
    strcat(mm, " ");
    di_convertencoding(mm);
    /* clear screen */
    di_drawrect(db, 0, 0, 255, 63, -1, 0x00);
    /* Now split the string at spaces */
    uint8_t * nhy = &mm[0];
    for (uint8_t lineno = 0; lineno < 2; lineno++) {
      uint8_t prevspace = 0;
      uint8_t curpos = 0;
      while (nhy[curpos] != 0) { // Not end of string
        if ((nhy[curpos] == ' ') && (curpos > 0)) { // a space!
          if (di_calctextwidth(&font_FreeSansBold21pt, nhy, curpos) <= 256) {
            /* This does fit! Remember this candidate! */
            prevspace = curpos;
          } else {
            break; /* If this doesn't fit, later ones can't either. */
          }
        }
        curpos++;
      }
      if (prevspace > 0) { /* We found a suitable space */
        nhy[prevspace] = 0; // replace space with Null terminator and print
        di_drawtext(db, 0, (lineno + 1) * 20, &font_FreeSansBold21pt, 0xff, nhy);
        nhy = &nhy[prevspace + 1]; // point at next segment.
      } else { /* no space found. */
        /* There are better ways to handle this, but for now, just print everything. */
        di_drawtext(db, 0, (lineno + 1) * 20, &font_FreeSansBold21pt, 0xff, nhy);
        nhy = nhy + strlen(nhy);
      }
    }
    /* Is there something remaining for line 3? */
    if (*nhy != 0) {
      di_drawtext(db, 0, 3 * 20, &font_FreeSansBold21pt, 0xff, nhy);
    }
    sh1122_display(db);
}

