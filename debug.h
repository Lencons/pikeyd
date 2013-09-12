/**** debug.h ******************************/
/*   Universal RPi GPIO keyboard daemon    */
/*                                         */
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

#ifndef _DEBUG_H_
#define _DEBUG_H_

#define DEBUG_INFO 1
#define DEBUG_GPIO 2
#define DEBUG_DEV1 5
#define DEBUG_DEV2 6
#define DEBUG_DEV3 7
#define DEBUG_DEV4 8
#define DEBUG_DEV5 9
#define DEBUG_ALL 10

extern int db_lvl;

void debug_init(int lvl);
int debug_lvl(void);
int debug_on(void);
void to_binstr(int val, char *str, int len);


#endif

