
/* Fetching / parsing data from regenampel.de */

#ifndef _REGENAMPELDE_H_
#define _REGENAMPELDE_H_

/* We fill this struct with data */
struct rade_data {
  int light_color;
  uint8_t message1[100];
  uint8_t message2[100];
  int valid;
};

/* Try to fetch an update. Returns 0 on success. */
int rade_tryupdate(struct rade_data * newdata);

#endif /* _REGENAMPELDE_H_ */

