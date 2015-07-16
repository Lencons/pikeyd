/**** gpio.c *******************************/
/*   Universal RPi GPIO keyboard daemon    */
/*                                         */
/* M. Moller   2013-01-16                  */
/* D. Lennox   2013-09-06                  */
/*******************************************/

/*
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "gpio.h"
#include "config.h"
#include "debug.h"

#define BOUNCE_TIME 2

#define GPIO_PERI_BASE        0x20000000
#define GPIO_BASE             (GPIO_PERI_BASE + 0x200000)
#define BLOCK_SIZE            (4 * 1024)
#define PAGE_SIZE             (4 * 1024)
#define GPIO_ADDR_OFFSET      13

struct joydata_struct
{
  int xio_mask;
  int is_xio[GPIO_NUM];
} joy_data[1];

/* GPIO setup macros.
 * These macros only work on the first 32 GPIO pins which is all we can use
 * on the RPi anyway.
 *
 * Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
 */
#define INP_GPIO(g) *(GPIO+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(GPIO+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(GPIO+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

/* register addresses for GPIO control */
#define GPIO_SET *(GPIO+7)	/* sets GPIO bits for mask */
#define GPIO_CLR *(GPIO+10)	/* clears GPIO bits for mask */
#define GPIO_PUD *(GPIO+37)	/* GPIO Pull up/down register */
#define GPIO_PUDCLK *(GPIO+38)	/* GPIO Pull up/down clock register */

/* GPIO pin ALT function set bits */
#define	GPIO_ALT_INPT		0b000	/* Pin as an input */
#define	GPIO_ALT_OUTP		0b001	/* Pin as an output */

volatile unsigned* GPIO;
static char GpioFlags[GPIO_NUM];
static int KeyRepeat = 0;

void handle_repeat(int grp, int gpio_state);


/* gpio_init() needs to be call once before the GPIO can be used */
int gpio_init(void) {

  int mem_fd;
  void *gpio_map;

  /* initalise default joy_data information */
  joy_data[0].xio_mask=0;

  /* memory map creation for direct GPIO access */
  if((mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0){
    perror("/dev/mem");
    printf("Failed to open memory\n");
    return(0);
  }

  gpio_map = mmap ( NULL,                    /* Any address */
                    BLOCK_SIZE,              /* map length */
                    PROT_READ | PROT_WRITE,  /* enable read & write */
                    MAP_SHARED,              /* share with other procces */
                    mem_fd,                  /* file to map */
                    GPIO_BASE );             /* offset to GPIO */
  close(mem_fd);

  if (gpio_map == MAP_FAILED) {
    perror("mmap");
    printf("Failed to map memory\n");
    return(0);
  }

  GPIO = (volatile unsigned*)gpio_map;
  memset(GpioFlags, 0, GPIO_NUM);


  if (debug_lvl() >= DEBUG_GPIO) {
    printf("GPIO mmap init OK.\n");
  }

  return(1);
}


/* set GPIO "pin" as either an input or output port */
int gpio_pincfg(int pin, char flg, int *mask) {

  char flg_str[16];
  int ret = 1;

  /* this is just for providing user readable output */
  switch (flg) {
    case GPIO_IN:
      strcpy(flg_str, "Input");
      break;
    case GPIO_OUT:
      strcpy(flg_str, "Output");
      break;
    default:
      strcpy(flg_str, "Unknown");
  }

  /* perform GPIO configuration for PIN */
  if (!GpioFlags[pin]) {
    switch (flg) {
      case GPIO_IN:
        INP_GPIO(pin);
        break;
      case GPIO_OUT:
        INP_GPIO(pin);
        OUT_GPIO(pin);
        GPIO_SET = 1<<pin;      /* default state for matrix driver */
        break;
      default:
        if (debug_lvl() >= DEBUG_GPIO) {
          printf("Invalid GPIO Flag: [%c]\nPin not configured.\n");
        }
        return(0);
    }
    GpioFlags[pin] = flg;

    if (debug_lvl() >= DEBUG_GPIO) {
      printf("GPIO%02d configured for %s\n", pin, flg_str);
    }
  }
  else if (GpioFlags[pin] == flg) {
    ret = 2;
    if (debug_lvl() >= DEBUG_GPIO) {
      printf("GPIO Configuration for GPIO%02d for %s skipped(%c)\n", pin, flg_str, GpioFlags[pin]);
    }
  }
  else {
    if (debug_lvl() >= DEBUG_GPIO) {
      printf("GPIO%02d already configured (%c), not changed to %s.\n", pin, GpioFlags[pin], flg_str);
    }
    return(-1);
  }

  if (mask) {
    (*mask) |= (1 << pin);
  }
  joy_data[0].xio_mask |= ( is_xio(pin) << pin );
  joy_data[0].is_xio[pin] = is_xio(pin);

  return(ret);
}


/* set the internal pull resisters for a pin */
int gpio_pull(int pin, int mode) {

  if (GpioFlags[pin] == GPIO_IN) {

    GPIO_PUD = mode & 3;
    usleep(5);
    GPIO_PUDCLK = 1 << (pin & 31);
    usleep(5);

    GPIO_PUD = 0;
    usleep(5);
    GPIO_PUDCLK = 0;
    usleep(5);

  }
  else {
    if (debug_lvl() >= DEBUG_GPIO) {
      char str[32];

      switch(mode) {
        case PUD_OFF:
          strcpy(str, "floating");
          break;

        case PUD_DOWN:
          strcpy(str, "pull down");
          break;

        case PUD_UP:
          strcpy(str, "pull up");
          break;

        default:
          strcpy(str, "UNKNOWN");
      }
      printf("GPIO%02d not set for %s as not configured for INPUT\n", pin, str);
    }
    return(0);
  }

  return(1);
}


void gpio_poll(int grp) {

  int i;
  int new_gpio;
  mat_grp_s *mat_grp;

  mat_grp = get_matgrp(grp);

  /* set driver pin LOW if this is a Matrix Group */
  if (mat_grp->gpio != -1) {
    GPIO_CLR = 1 << mat_grp->gpio;
    usleep(5);
  }

  /* get current GPIO pin states */
  new_gpio = ((int*)GPIO)[GPIO_ADDR_OFFSET] & mat_grp->gpio_mask;

  /* driver pin HIGH if it was set for the Matrix Group */
  if (mat_grp->gpio != -1) {
    GPIO_SET = 1 << mat_grp->gpio;
  }

  if (new_gpio != mat_grp->last_gpio) {
    mat_grp->bounce_cnt = 0;
    mat_grp->cur_gpio |= new_gpio ^ mat_grp->last_gpio;
  }
  mat_grp->last_gpio = new_gpio;

  if (debug_lvl() >= DEBUG_DEV5) {
    printf("GRP[%d]GPIO State: %08x, Pin Mask: %08x, Pending: %08x\n", grp, new_gpio, mat_grp->gpio_mask, mat_grp->cur_gpio);
  }

  if (mat_grp->bounce_cnt >= BOUNCE_TIME) {

    for (i=0; i<GPIO_NUM; i++) {

      /* handle IRQs */
      if (~new_gpio & joy_data[0].xio_mask) {
        if (is_xio(i) & !(new_gpio & (1<<i))) {
          send_gpio_keys(grp, i);
        }
      }

      /* handle GPIO buttons */
      if (mat_grp->cur_gpio & (1<<i)) {

        /* only send a key on press (pin low) */
        if (!(new_gpio & (1<<i))) {
          send_gpio_keys(grp, i);
        }
      }
    }
    mat_grp->cur_gpio = 0;
  }

  if (mat_grp->bounce_cnt < BOUNCE_TIME) {
    mat_grp->bounce_cnt++;
  }

  handle_repeat(grp, (~new_gpio) & mat_grp->gpio_mask);

  return;
}


void force_repeat(void) {

  KeyRepeat = 1;

  return;
}


void handle_repeat(int grp, int gpio_state) {

  /* key repeat metrics: release after 80ms, press after 200ms, release after 40ms, press after 40ms */
  const struct {
    int time[4];
    int value[4];
    int next[4];
  }mxkey = {
    {80, 200, 40, 40},
    {0, 1, 0, 1},
    {1, 2, 3, 2}
  };

  int i, g;
  mat_grp_s *mat_grp;

  mat_grp = get_matgrp(grp);

  g = gpio_state & mat_grp->prev_gpio;
  mat_grp->prev_gpio = gpio_state;

  for (i=0; i<NUM_GPIO; i++) {
    if ((mat_grp->rpt_flg & (1<<i)) || KeyRepeat) {
      if (g & (1<<i)) {

        /* we get called about every 4ms */ 
        mat_grp->key_rpt[i].t_now += 4;

        if (mat_grp->key_rpt[i].idx == -1) {
          mat_grp->key_rpt[i].idx = 0;
          mat_grp->key_rpt[i].t_next = mat_grp->key_rpt[i].t_now + mxkey.time[mat_grp->key_rpt[i].idx];
          mat_grp->key_rpt[i].t_now = 0;
        }
        else if (mat_grp->key_rpt[i].t_now >= mat_grp->key_rpt[i].t_next) {
          send_gpio_keys(grp, i);
          mat_grp->key_rpt[i].idx = mxkey.next[mat_grp->key_rpt[i].idx];
          mat_grp->key_rpt[i].t_next = mxkey.time[mat_grp->key_rpt[i].idx];
          mat_grp->key_rpt[i].t_now = 0;
        }
      }
      /* non-repeat state, reset repeat values */
      else {
        mat_grp->key_rpt[i].idx = -1;
        mat_grp->key_rpt[i].t_now = 0;
        mat_grp->key_rpt[i].t_next = 0;
      }
    }
  }

  return;
}

