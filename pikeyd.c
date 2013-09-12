/**** pikeyd.c *****************************/
/*   Universal RPi GPIO keyboard daemon    */
/*                                         */
/* M. Moller   2013-01-16                  */
/* D. Lennox   2013-08-25                  */
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
#include "gpio.h"
#include "iic.h"
#include "debug.h"

void showHelp(void);
void showVersion(void);

int main(int argc, char *argv[])
{
  int en_daemonize = 0;
  int i;

  for(i=1; i<argc; i++){
    if (!strcmp(argv[i], "-d")) {
      en_daemonize = 1;
      //daemonize("/tmp", "/tmp/pikeyd.pid");
    }
    else if (!strcmp(argv[i], "-k")) {
      daemonKill("/tmp/pikeyd.pid");
      exit(0);
    }
    else if (!strcmp(argv[i], "-r")) {
      /* TODO: repeat broken with addition of matrix groups */
      printf("Key repeat currently disabled.\n");
      //joy_enable_repeat();
    }
    else if (!strcmp(argv[i], "-v")) {
      showVersion();
      exit(0);
    }
    else if (!strcmp(argv[i], "-h")) {
      showHelp();
      exit(0);
    }
    /* debug options */
    else if (!strncmp(argv[i], "-D", 2)) {
      int d = 1;
      char *p = &argv[i][2];

      if (argv[i][2]) {
        d = strtol(&argv[i][2], &p, 10);
        if (d > 10) {
          d = 10;
        }
      }
      if (*p) {
        printf("Unknown -D option: %s\n", argv[i]);
        exit(-1);
      }
      debug_init(d);
      printf("DEBUG LEVEL %d\n", d);
    }
    else {
      printf("Unknown command line argument: %s\n", argv[i]);
      showHelp();
      exit(-1);
    }
  }

  if(en_daemonize){
    daemonize("/tmp", "/tmp/pikeyd.pid");
  }

  if (!gpio_init()) {
    return(-1);
  }

  switch (init_config()) {
    case 0:
      return(-1);
    case 1:
      break;
    case 2:
      init_iic();
  }

  printf("Ready.\n");

  //test_iic(0x20);  close_iic();  exit(0);

  if(!init_uinput()){
    sleep(1);

    if(!en_daemonize){
      printf("Press ^C to exit.\n");
    }

    while (1) {
      for (i=0; i<=mat_count(); i++) {
        gpio_poll(i);
      }
      usleep(4000);
    }
  }

  return 0;
}

void showHelp(void)
{
  printf("Usage: pikeyd [option]\n");
  printf("Options:\n");
  printf("  -d    run as daemon\n");
  printf("  -D?   debug level (-D1 to -D10)\n");
  printf("  -r    force key repeats\n");
  printf("  -k    try to terminate running daemon\n");
  printf("  -v    version\n");
  printf("  -h    this help\n");
}

void showVersion(void)
{
  printf("pikeyd 1.3 (May 2013)\n");
  printf("The Universal Raspberry Pi GPIO keyboard daemon.\n");
  printf("This is free software; see the source for copying conditions.  There is NO\n");
  printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
}
