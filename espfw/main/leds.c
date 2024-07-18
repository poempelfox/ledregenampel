
#include <driver/ledc.h>
#include <esp_log.h>
#include "leds.h"
#include "settings.h"

/* Red is connected on GPIO7, Yellow on GPIO9, and Green on GPIO8 */
#define GPIOLEDRED 7
#define GPIOLEDYEL 9
#define GPIOLEDGRE 8

void leds_init(void)
{
  ledc_timer_config_t ltc = {
    .speed_mode = LEDC_LOW_SPEED_MODE, /* C3 only supports low speed mode */
    .duty_resolution = LEDC_TIMER_12_BIT, /* 12 bit resolution, i.e. 0-4095 */
    .timer_num = LEDC_TIMER_0, /* timer source */
    .freq_hz = 2000, /* not sure what's best here yet. 1000 flickers quite bad visibly. */
    .clk_cfg = LEDC_AUTO_CLK, /* FIXME to check: does this select a DFS compatible clock? */
  };
  esp_err_t e = ledc_timer_config(&ltc);
  if (e != ESP_OK) {
    ESP_LOGE("leds.c", "ERROR: Failed to configure LEDC timer: %s", esp_err_to_name(e));
  }
  ledc_channel_config_t lcc = { /* Common for all our channels */
    .speed_mode = LEDC_LOW_SPEED_MODE, /* C3 only supports low speed mode */
    .intr_type = LEDC_INTR_DISABLE,
    .timer_sel = LEDC_TIMER_0,
    .duty = 0, /* duty cycle 0 a.k.a. off for now */
    .hpoint = 0, /* we don't need to spread the load because we only use one LED at a time, so offset 0 is fine */
    //.output_invert = 0,
  };
  /* Now use this 3 times to configure our 3 LED channels */
  lcc.gpio_num = GPIOLEDRED;
  lcc.channel = LED_RED;
  e = ledc_channel_config(&lcc);
  if (e != ESP_OK) {
    ESP_LOGE("leds.c", "ERROR: Failed to configure LEDC channel 0 / red: %s", esp_err_to_name(e));
  }
  lcc.gpio_num = GPIOLEDYEL;
  lcc.channel = LED_YELLOW;
  e = ledc_channel_config(&lcc);
  if (e != ESP_OK) {
    ESP_LOGE("leds.c", "ERROR: Failed to configure LEDC channel 1 / yellow: %s", esp_err_to_name(e));
  }
  lcc.gpio_num = GPIOLEDGRE;
  lcc.channel = LED_GREEN;
  e = ledc_channel_config(&lcc);
  if (e != ESP_OK) {
    ESP_LOGE("leds.c", "ERROR: Failed to configure LEDC channel 2 / green: %s", esp_err_to_name(e));
  }
}

void leds_setbrightness(uint8_t led, uint16_t brightness)
{
  ledc_set_duty(LEDC_LOW_SPEED_MODE, led, brightness);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, led);
}

void leds_setledon(uint8_t led)
{
  for (int i = 0; i <= 2; i++) {
    if (led == i) { /* Turn this one on */
      leds_setbrightness(i, settings.ledn_bri[i]);
    } else { /* Turn this one off */
      leds_setbrightness(i, 0);
    }
  }
}

