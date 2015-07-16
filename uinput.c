/**** uinput.c *****************************/
/*   Universal RPi GPIO keyboard daemon    */
/*                                         */
/* M. Moller   2013-01-16                  */
/* D. Lennox   2013-09-14                  */
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
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include "config.h"
#include "uinput.h"
#include "debug.h"

int sendRel(int dx, int dy);
int sendSync(void);

static struct input_event     uidev_ev;
static int uidev_fd;
static keyinfo_s lastkey;

#define die(str, args...) do { \
        perror(str); \
        return(EXIT_FAILURE); \
    } while(0)

int init_uinput(void) {

  int fd;
  struct uinput_user_dev uidev;
  int i;

  fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
  if(fd < 0)
    die("/dev/uinput");

  if(ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0)
    die("error: ioctl");
  if(ioctl(fd, UI_SET_EVBIT, EV_REP) < 0)
    die("error: ioctl");
  if(ioctl(fd, UI_SET_KEYBIT, BTN_LEFT) < 0)
    die("error: ioctl");

  /* don't forget to add all the keys! */
  for(i=0; i<256; i++){
    if(ioctl(fd, UI_SET_KEYBIT, i) < 0)
      die("error: ioctl");
  }

  memset(&uidev, 0, sizeof(uidev));
  snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "pikeyd");
  uidev.id.bustype = BUS_USB;
  uidev.id.vendor  = 0x1;
  uidev.id.product = 0x1;
  uidev.id.version = 1;

  if(write(fd, &uidev, sizeof(uidev)) < 0)
    die("error: write");

  if(ioctl(fd, UI_DEV_CREATE) < 0)
    die("error: ioctl");

  uidev_fd = fd;

  return(1);
}


int close_uinput(void)
{
  sleep(2);

  if(ioctl(uidev_fd, UI_DEV_DESTROY) < 0)
    die("error: ioctl");

  close(uidev_fd);

  return(1);
}


int sendKey(int key, int value)
{
  memset(&uidev_ev, 0, sizeof(struct input_event));
  gettimeofday(&uidev_ev.time, NULL);
  uidev_ev.type = EV_KEY;
  uidev_ev.code = key;
  uidev_ev.value = value;

  if (debug_lvl() >= DEBUG_DEV4) {
    printf("sendKey: %d = %d\n", key, value);
  }

  if(write(uidev_fd, &uidev_ev, sizeof(struct input_event)) < 0)
    die("error: write");

  sendSync();

  return 0;
}


int sendRel(int dx, int dy)
{
  memset(&uidev_ev, 0, sizeof(struct input_event));
  uidev_ev.type = EV_REL;
  uidev_ev.code = REL_X;
  uidev_ev.value = dx;
  if(write(uidev_fd, &uidev_ev, sizeof(struct input_event)) < 0)
    die("error: write");

  memset(&uidev_ev, 0, sizeof(struct input_event));
  uidev_ev.type = EV_REL;
  uidev_ev.code = REL_Y;
  uidev_ev.value = dy;
  if(write(uidev_fd, &uidev_ev, sizeof(struct input_event)) < 0)
    die("error: write");

  sendSync();

  return 0;
}


int sendSync(void)
{
  memset(&uidev_ev, 0, sizeof(struct input_event));
  uidev_ev.type = EV_SYN;
  uidev_ev.code = SYN_REPORT;
  uidev_ev.value = 0;
  if(write(uidev_fd, &uidev_ev, sizeof(struct input_event)) < 0)
    die("error: sendSync - write");

  return 0;
}


int send_gpio_keys(int grp, int gpio) {

  int k;
  int xio;

  restart_keys(grp);
  while( got_more_keys(grp, gpio) ){
    k = get_next_key(grp, gpio);
    if(is_xio(gpio)){
      xio = get_curr_xio_no();
      poll_iic(xio);
    }
    else if(k<0x300){
      sendKey(k, 1);
      sendKey(k, 0);
    }
  }
  return k;
}

