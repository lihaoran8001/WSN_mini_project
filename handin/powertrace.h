#ifndef POWERTRACE_H
#define POWERTRACE_H

#include "sys/clock.h"

void powertrace_start(clock_time_t perioc);
void powertrace_stop(void);

typedef enum {
  POWERTRACE_ON,
  POWERTRACE_OFF
} powertrace_onoff_t;

void powertrace_sniff(powertrace_onoff_t onoff);

void powertrace_print(char *str);

#endif /* POWERTRACE_H */
