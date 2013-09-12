/**** config.h *****************************/
/* M. Moller   2013-01-16                  */
/*   Universal RPi GPIO keyboard daemon    */
/*******************************************/

/*
   Copyright (C) 2013 Michael Moller.
   This file is part of the Universal Raspberry Pi GPIO keyboard daemon.

   This is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The software is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  
*/

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "iic.h"

#define NUM_GPIO 32

typedef struct {
  char name[32];
  int code;
} key_names_s;

typedef struct {
  int key;
  int val;
} keyinfo_s;

typedef struct _gpio_key {
  int gpio;
  int idx;
  int key;
  int xio; /* -1 for direct gpio */
  struct _gpio_key *next;
} gpio_key_s;

/* matrix group */
typedef struct {
  char *name;                     /* user defined group name, NULL for direct */
  int gpio;                       /* GPIO driver pin for matrix */
  int gpio_mask;                  /* input mask for group pins */
  int cur_gpio;                   /* GPIO state currently being processed */
  int last_gpio;                  /* GPIO state at last read */
  int bounce_cnt;                 /* switch bounce delay counter */
  gpio_key_s *gpio_key[NUM_GPIO];
  gpio_key_s *last_gpio_key;
} mat_grp_s;

int init_config(void);
int get_event_key(int gpio, int idx);
int get_next_key(int grp, int gpio);
int got_more_keys(int grp, int gpio);
int got_more_xio_keys(int xio, int gpio);
void restart_keys(int grp);
int gpio_pin(int n);
int is_xio(int gpio);
int get_curr_key(int grp);
int get_curr_xio_no(void);
void get_xio_parm(int xio, iodev_e *type, int *addr, int *regno);
int get_next_xio_key(int xio, int gpio);
void restart_xio_keys(int xio);
void handle_iic_event(int xio, int value);
void last_iic_key(keyinfo_s *kp);
mat_grp_s *get_matgrp(int grp);
int mat_count(void);

#endif

