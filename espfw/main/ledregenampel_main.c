/* LED-Regen-Ampel. The main structure of this is stolen from foxesptemp.
 */
#include "sdkconfig.h"
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <driver/i2c.h>
#include <soc/i2c_reg.h>
#include <time.h>
#include <esp_sntp.h>
#include <nvs_flash.h>
#include <esp_ota_ops.h>
#include <math.h>
#include <esp_netif.h>
#include <sys/time.h>
#include "display.h"
#include "i2c.h"
#include "leds.h"
#include "network.h"
#include "regenampelde.h"
#include "settings.h"
#include "sh1122.h"
#include "webserver.h"

#define sleep_ms(x) vTaskDelay(pdMS_TO_TICKS(x))

/* Global / Exported variables, used to provide the webserver.
 * struct ev is defined in webserver.h for practical reasons. */
struct ev evs[2];
int activeevs = 0;
static const char *TAG = "ledregenampel";

/* Has the firmware been marked as "good" yet, or is ist still pending
 * verification? */
int pendingfwverify = 0;

/* we need this to display our IP, it is in network.c */
extern esp_netif_t * mainnetif;

/* Our main display buffer (we currently only use one) */
struct di_dispbuf * db;

/* Last data received from regenampel.de */
struct rade_data rad = { .light_color = LED_INVALID };

/* regenampel.de updateinterval. We might want to move this to settings? */
int raupdateint = 300;

/* This variable is normally 0. The webserver will set it to something
 * in order to tell us, we need to blink all lights once, because the
 * user clicked 'Test' on the webinterface. */
uint8_t ledoverride = 0;

void app_main(void)
{
    /* This is in all OTA-Update examples, so I consider it mandatory. */
    esp_err_t err = nvs_flash_init();
    if ((err == ESP_ERR_NVS_NO_FREE_PAGES) || (err == ESP_ERR_NVS_NEW_VERSION_FOUND)) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    /* Load settings from NVRAM */
    settings_hardcode(); /* smuggles in hardcoded settings for testing */
    settings_load();

    /* Already try to start up the network (will happen mostly in the BG) */
    network_prepare();
    network_on(); /* We don't do network_off, we just try to stay connected */

    i2c_port_init();
    sh1122_init();
    db = di_newdispbuf();
    di_drawtext(db, 10, 30, &font_FreeSansBold21pt, 0xff, "Foxis LED-Regen-Ampel");
    sh1122_display(db);

    leds_init();

    /* Unfortunately, time does not (always) revert to 0 on an
     * esp_restart. So we set all timestamps to "now" instead.
     * We push back lastupdatt so an update attempt is made
     * after about 10 seconds. */
    time_t lastupdatt = time(NULL) - (raupdateint - 10);
    time_t lastupdsuc = time(NULL);

    /* We do NTP to provide useful timestamps in our webserver output. */
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "ntp2.fau.de");
    esp_sntp_setservername(1, "ntp3.fau.de");
    esp_sntp_init();

    /* In case we were OTA-updating, we set this fact in a variable for the
     * webserver. Someone will need to click "Keep this firmware" in the
     * webinterface to mark the updated firmware image as good, otherwise we
     * will roll back to the previous and known working version on the next
     * reset. */
    const esp_partition_t * running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
      if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
        pendingfwverify = 1;
      }
    }

    /* Start up the webserver */
    webserver_start();

    /* Wait for up to 7 more seconds to connect to WiFi and get an IP */
    EventBits_t eb = xEventGroupWaitBits(network_event_group,
                                         NETWORK_CONNECTED_BIT,
                                         pdFALSE, pdFALSE,
                                         (7000 / portTICK_PERIOD_MS));
    if ((eb & NETWORK_CONNECTED_BIT) == NETWORK_CONNECTED_BIT) {
      ESP_LOGI("main.c", "Successfully connected to network.");
    } else {
      ESP_LOGW("main.c", "Warning: Could not connect to WiFi. This is probably not good.");
    }

    /* Now loop, polling regenampel.de every 5 minutes or so. */
    while (1) {
      if (lastupdatt > time(NULL)) { /* This should not be possible, but we've
         * seen time(NULL) temporarily report insane timestamps far in the
         * future - not sure why, possibly after a watchdog reset, but in any
         * case, lets try to work around it: */
        ESP_LOGE(TAG, "Lets do the time warp again: Time jumped backwards - trying to cope.");
        lastupdatt = time(NULL); // We should return to normality on next iteration
      }
      time_t curts = time(NULL);
      if ((curts - lastupdatt) >= raupdateint) {
        lastupdatt = curts;
        int naevs = (activeevs == 0) ? 1 : 0;
        evs[naevs].lastupdatt = lastupdatt;
        ESP_LOGI("main.c", "Trying to update from regenampel.de...");
        struct rade_data nrad;
        if (rade_tryupdate(&nrad) == 0) { // Successful update
          lastupdsuc = curts;
          evs[naevs].lastupdsuc = lastupdsuc;
          rad = nrad;
        } else { // No successful update
          /* Can we still reuse the old data? */
          if ((curts - lastupdsuc) > (2 * raupdateint + 10)) {
            // data is more than 2 update-intervals old, we no longer accept that.
            // So null the data.
            rad.light_color = LED_INVALID;
            strcpy(rad.message1, "No current data :(");
            strcpy(rad.message2, "No current data :(");
          }
        }
        leds_setledon(rad.light_color);
        di_updateoledwith2msgs(db, rad.message1, rad.message2);
        /* invert the display for the second 15 minutes
         * of every half hour to guarantee equal pixel aging */
        sh1122_setinvertmode( ((curts % 1800) > 900) );
        evs[naevs].light_color = rad.light_color;
        strcpy(evs[naevs].message1, rad.message1);
        strcpy(evs[naevs].message2, rad.message2);

        /* Now mark the updated values as the current ones for the webserver */
        activeevs = naevs;

        if ((curts > 900)
         && ((curts - lastupdsuc) > 900)
         && ((curts - lastupdsuc) < 100000)) {
          /* We have been up for at least 15 minutes and not
           * successfully submitted any values in more than
           * 15 minutes (but the timespan isn't ultralong either,
           * which happens when NTP jumps our time).
           * It might be a good idea to reboot. */
          ESP_LOGE(TAG, "No successful update in %lld seconds - about to reboot.", (time(NULL) - lastupdsuc));
          esp_restart();
        }
      }
      if (ledoverride != 0) { // blink all LEDs in sequence.
        if (ledoverride > 3) { ledoverride = 3; }
        leds_setledon(2 - (ledoverride - 1));
        ledoverride--;
        sleep_ms(1000);
        if ((ledoverride == 0) && (rad.light_color != LED_INVALID)) {
          // restore old state so we don't have to wait for next update.
          // This is irrelevant if we're blinking anyways...
          leds_setledon(rad.light_color);
        }
      } else {
        if (rad.light_color == LED_INVALID) { // Blink yellow
          struct timeval ctv;
          gettimeofday(&ctv, NULL);
          if (ctv.tv_usec > 500000) { // later half of a second
            leds_setledon(LED_YELLOW);
          } else { // earlier half of a second
            leds_setledon(0xff);
          }
          /* Only do a short sleep so we can blink */
          sleep_ms(100);
        } else {
          /* Nothing to do but go back to sleep for a second. */
          /* We sadly cannot do meaningful powersaving here if we want the
           * webinterface to be reachable. */
          sleep_ms(1000);
        }
      }
    }
}

