
#ifndef _FONT_H_
#define _FONT_H_

#include <inttypes.h>

/* We're now using the same font structure as the Adafruit_GFX
 * library, mainly so we could reuse their font converter and
 * didn't have to develop our own from scratch. */

/// Font data stored PER GLYPH
typedef struct
{
    uint32_t bitmapOffset; ///< Pointer into GFXfont->bitmap
    uint8_t width;         ///< Bitmap dimensions in pixels
    uint8_t height;        ///< Bitmap dimensions in pixels
    uint8_t xAdvance;      ///< Distance to advance cursor (x axis)
    int xOffset;           ///< X dist from cursor pos to UL corner
    int yOffset;           ///< Y dist from cursor pos to UL corner
} GFXglyph;

/// Data stored for FONT AS A WHOLE
struct font
{
    uint8_t *bitmap; ///< Glyph bitmaps, concatenated
    GFXglyph *glyph; ///< Glyph array
    uint8_t first;   ///< ASCII extents (first char)
    uint8_t last;    ///< ASCII extents (last char)
    int yAdvance;    ///< Newline distance (y axis)
};

typedef struct font GFXfont;

#endif /* _FONT_H_ */

