/**** gpio.h *******************************/
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

#ifndef _GPIO_H_
#define _GPIO_H_

#define GPIO_NUM	32
#define GPIO_IN		'I'
#define GPIO_OUT        'O'

/* GPIO Pull up/down resister modes */
#define PUD_OFF                 0       /* disable pull up/down */
#define PUD_DOWN                1       /* enable pull down resister */
#define PUD_UP                  2       /* enable pull up resister */

int gpio_init(void);
int gpio_pincfg(int pin, char flg, int *mask);
int gpio_pull(int pin, int mode);
void gpio_poll(int grp);
void force_repeat(void);

#endif


