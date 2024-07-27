
/* Builtin Webserver */

#ifndef _WEBSERVER_H_
#define _WEBSERVER_H_

/* This struct is used to provide data to us */
struct ev {
  time_t lastupdatt;
  time_t lastupdsuc;
  int light_color;
  uint8_t message1[100];
  uint8_t message2[100];
};

/* Initialize and start the Webserver. */
void webserver_start(void);

#endif /* _WEBSERVER_H_ */

