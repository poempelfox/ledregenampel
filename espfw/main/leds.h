
#ifndef _LEDS_H_
#define _LEDS_H_

/* defines to use as the 'led' parameter in setledbrightness().
 * These are simply the channel numbers for the esp-idf functions. */
#define LED_RED 0
#define LED_YELLOW 1
#define LED_GREEN 2

/* initializes the LED pins */
void leds_init(void);

/* selects which LED to turn on. "0xff" to turn all off. */
void leds_setledon(uint8_t led);

#endif /* _LEDS_H_ */

