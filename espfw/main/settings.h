
/* Global variables containing main settings, and
 * helper functions for loading/writing settings.
 */

#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#define WIFIMODE_AP 0
#define WIFIMODE_CL 1

struct globalsettings {
	/* WiFi settings */
	uint8_t wifi_mode; /* AP or CLient */
	uint8_t wifi_cl_ssid[33];
	uint8_t wifi_cl_pw[64];
	uint8_t wifi_ap_ssid[33];
	uint8_t wifi_ap_pw[64];
	/* Password for the Admin pages in the Webinterface */
	uint8_t adminpw[25];
        /* The brightness for each of the three LED channels
         * (for PWM, so 0-4095) */
        uint32_t ledn_bri[3];
	/* regenampel.de request string */
	uint8_t radereqstr[256];
        /* ...and prewarntime (how long before rain the light goes yellow) */
        uint8_t radeprewarntime;
};

extern struct globalsettings settings;

/* Load main settings */
void settings_load(void);

void settings_hardcode(void);

#endif /* _SETTINGS_H_ */

