
#ifndef _LEDS_H_
#define _LEDS_H_

/* defines to use as the 'led' parameter in the functions below.
 * These are simply the channel numbers for the esp-idf functions. */
#define LED_RED 0
#define LED_YELLOW 1
#define LED_GREEN 2

/* Initializes the LED pins / GPIOs.
 * Needs to be called once before calling any of the other functions. */
void leds_init(void);

/* Turns on the one selected LED, and off all others.
 * Use "0xff" (or any value greater than the number of LEDs) to turn all off. */
void leds_setledon(uint8_t led);

/* Manually set the brightness of one of the LEDs.
 * Any subsequent call to leds_setledon() will clear this.
 * As we use 12 bit PWM, valid values for brightness are
 * in the range [ 0 ... 4095 ] */
void leds_setbrightness(uint8_t led, uint16_t brightness);

#endif /* _LEDS_H_ */

