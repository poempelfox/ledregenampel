
#include <stdlib.h>
#include <string.h>
#include <esp_log.h>
#include <esp_http_client.h>
#include <esp_crt_bundle.h>
#include "regenampelde.h"
#include "settings.h"

struct httprcvbuf {
  int bpos;
  uint8_t * buf;
};

/* It doesn't seem to be possible to do a FSCKing
 * simple HTTP GET of a few bytes with ESP-IDF bultins.
 * _THE_ most basic simple absolutely mandatory
 * functionality of a HTTP client.
 * One has to add an event handler and buffer the whole
 * reply themselves. WTF. This is unbelievably braindead.
 */
esp_err_t letsimplementbasicfunctionalitybecauseespressifislazy(esp_http_client_event_handle_t e)
{
  if (e->event_id == HTTP_EVENT_ON_DATA) {
    if (e->data_len > 0) { // We received some data!
      struct httprcvbuf * rb = (struct httprcvbuf *)e->user_data;
      int newbsize = rb->bpos + e->data_len + 1;
      if (newbsize > 8000) { // sanity check: We don't expect to receive something that large.
        ESP_LOGE("regenampelde.c", "Received HTTP reply to large, dropping end of it.");
        return ESP_OK;
      }
      if (rb->buf == NULL) {
        rb->buf = malloc(newbsize);
        if (rb->buf == NULL) { // malloc / realloc failled. This is bad.
          ESP_LOGE("regenampelde.c", "OOM receiving HTTP data (1)");
          return ESP_OK;
        }
      } else {
        uint8_t * oldrbbuf = rb->buf;
        rb->buf = realloc(rb->buf, newbsize);
        if (rb->buf == NULL) { // malloc / realloc failled. This is bad.
          free(oldrbbuf);
          rb->bpos = 0;
          ESP_LOGE("regenampelde.c", "OOM receiving HTTP data (1)");
          return ESP_OK;
        }
      }
      memcpy(&rb->buf[rb->bpos], e->data, e->data_len);
      rb->bpos += e->data_len;
    }
  }
  return ESP_OK;
}

int rade_tryupdate(struct rade_data * newdata)
{
  newdata->valid = 0;
  newdata->light_color = 0;
  strcpy(newdata->message1, "");
  strcpy(newdata->message2, "");
  if (strlen(settings.radereqstr) < 18) { // This cannot be a valid request.
    return 1;
  }
  uint8_t mergedurl[600]; // radereqstr is at most 256 bytes so this should be enough
  strcpy(mergedurl, "https://wetter.poempelfox.de/api/radeaa/?dataformat=txt&");
  if (settings.radeprewarntime > 0) {
    uint8_t pwtstr[20];
    sprintf(pwtstr, "%d", settings.radeprewarntime);
    strcat(mergedurl, "prewarntime=");
    strcat(mergedurl, pwtstr);
    strcat(mergedurl, "&");
  }
  strcat(mergedurl, settings.radereqstr);
  ESP_LOGI("regenampelde.c", "Requesting: %s", mergedurl);
  struct httprcvbuf resp = { .bpos = 0, .buf = NULL };
  esp_http_client_config_t cc = {
    .url = &mergedurl[0],
    .crt_bundle_attach = esp_crt_bundle_attach,
    .timeout_ms = 5000,
    .user_agent = "Foxis LED-Regenampel (ESP32)",
    .event_handler = letsimplementbasicfunctionalitybecauseespressifislazy,
    .user_data = (void *)&resp
  };
  esp_http_client_handle_t httpc = esp_http_client_init(&cc);
  esp_err_t err = esp_http_client_perform(httpc);
  if (err != ESP_OK) {
    esp_http_client_cleanup(httpc);
    if (resp.buf != NULL) { free(resp.buf); }
    ESP_LOGE("regenampelde.c", "Error getting data: %s", esp_err_to_name(err));
    return 1;
  }
  if (esp_http_client_get_status_code(httpc) != 200) {
    esp_http_client_cleanup(httpc);
    if (resp.buf != NULL) { free(resp.buf); }
    ESP_LOGE("regenampelde.c", "got HTTP status code %d", esp_http_client_get_status_code(httpc));
    return 1;
  }
  if (resp.bpos < 10) {
    esp_http_client_cleanup(httpc);
    if (resp.buf != NULL) { free(resp.buf); }
    ESP_LOGE("regenampelde.c", "empty response received from API");
    return 1;
  }
  resp.buf[resp.bpos] = 0; // Null-terminate the string
  esp_http_client_cleanup(httpc);
  //ESP_LOGI("regenampel.de", "Received response: %s", resp.buf);
  char * stmp;
  char * ni = strtok_r(resp.buf, "\n", &stmp);
  while (ni != NULL) {
    // Handle this line
    //ESP_LOGI("regenampelde.c", "Handling line: %s", ni);
    if (strncmp(ni, "LightColor=", 11) == 0) {
      int c = *(ni + 11) - '1';
      if ((c < 0) || (c > 2)) {
        ESP_LOGW("regenampelde.c", "Invalid LightColor: %s", (ni + 11));
      } else {
        newdata->light_color = c;
        newdata->valid = 1;
      }
    }
    if (strncmp(ni, "Message1=", 9) == 0) {
      strlcpy(newdata->message1, (ni + 9), sizeof(newdata->message1));
      newdata->valid = 1;
    }
    if (strncmp(ni, "Message2=", 9) == 0) {
      strlcpy(newdata->message2, (ni + 9), sizeof(newdata->message2));
      newdata->valid = 1;
    }
    ni = strtok_r(NULL, "\n", &stmp);
  }
  free(resp.buf);
  return 0;
}

