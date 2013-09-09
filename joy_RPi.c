/**** joy_RPi.c ****************************/
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

/*******************************************/
/* based on the xmame driver by
/* Jason Birch   2012-11-21   V1.00        */
/* Joystick control for Raspberry Pi GPIO. */
/*******************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "joy_RPi.h"
#include "config.h"
#include "debug.h"

#define GPIO_PERI_BASE        0x20000000
#define GPIO_BASE             (GPIO_PERI_BASE + 0x200000)
#define BLOCK_SIZE            (4 * 1024)
#define PAGE_SIZE             (4 * 1024)
#define GPIO_ADDR_OFFSET      13

#define BOUNCE_TIME 2

static int GpioFile;
static char* GpioMemory;
static char* GpioMemoryMap;
volatile unsigned* GPIO;
static char GpioFlags[GPIO_NUM];
static int AllMask;
static int lastGpio=0;
static int xGpio=0;
static int bounceCount=0;
static int doRepeat=0;

struct joydata_struct
{
  int num_buttons;
  int button_mask;
  int change_mask;
  int xio_mask;
  int buttons[GPIO_NUM];
  int change[GPIO_NUM];
  int is_xio[GPIO_NUM];
} joy_data[1];


int gpio_init(void)
{

  /* initalise default joy_data information */
  joy_data[0].num_buttons = 0;
  joy_data[0].button_mask=0;
  joy_data[0].xio_mask=0;

  memset(GpioFlags, 0, GPIO_NUM);

  /* memory map creation for direct GPIO access */
  GPIO = NULL;
  GpioMemory = NULL;
  if((GpioFile = open("/dev/mem", O_RDWR | O_SYNC)) < 0){
    perror("/dev/mem");
    printf("Failed to open memory\n");
    return(0);
  }
  else if(!(GpioMemory = malloc(BLOCK_SIZE + PAGE_SIZE - 1))){
    perror("malloc");
    printf("Failed to allocate memory map\n");
    return(0);
  }
  else{
    if ((unsigned long)GpioMemory % PAGE_SIZE){
      GpioMemory += PAGE_SIZE - ((unsigned long)GpioMemory % PAGE_SIZE);
    }

    if ((long)(GpioMemoryMap = 
	       (unsigned char*)mmap(
				    (caddr_t)GpioMemory, 
				    BLOCK_SIZE, 
				    PROT_READ | PROT_WRITE,
				    MAP_SHARED | MAP_FIXED, 
				    GpioFile, 
				    GPIO_BASE
				    )
	       ) < 0){
      perror("mmap");
      printf("Failed to map memory\n");
      close(GpioFile);
      return(0);
    }
    else{
      close(GpioFile);
      GPIO = (volatile unsigned*)GpioMemoryMap;
      lastGpio = ((int*)GPIO)[GPIO_ADDR_OFFSET];
    }
  }

  printf("Joystick init OK.\n");

  return(1);
}


int gpio_pincfg(int pin, char flg) {

  FILE *fp;
  char str[BUFSIZ];
  char flg_str[16];

  /* perform GPIO configuration for PIN */
  if (!GpioFlags[pin]) {
    sprintf(str, "/sys/class/gpio/export");
    if (!(fp = fopen(str, "w"))){
      perror(str);
      return(0);
    }
    fprintf(fp, "%u", gpio_pin(pin));
    fclose(fp);

    /* set the pin I/O direction as INPUT */
    sprintf(str, "/sys/class/gpio/gpio%u/direction", gpio_pin(pin));
    if (!(fp = fopen(str, "w"))){
      perror(str);
      return(0);
    }
    switch (flg) {
      case GPIO_IN:
        fprintf(fp, "in");
        strcpy(flg_str, "Input");
        break;
      case GPIO_OUT:
        fprintf(fp, "out");
        strcpy(flg_str, "Output");
        break;
      default:
        if (debug_on() >= DEBUG_GPIO) {
          printf("Invalid GPIO Flag: [%c]\nPin not configured.\n");
        }
        fclose(fp);
        return(0);
    }
    fclose(fp);
    GpioFlags[pin] = flg;

    AllMask |= (1 << gpio_pin(pin));
    joy_data[0].xio_mask |= ( is_xio(gpio_pin(pin)) << gpio_pin(pin) );
    joy_data[0].is_xio[pin] = is_xio(gpio_pin(pin));

    if (debug_on() >= DEBUG_GPIO) {
      printf("GPIO%02d configured for %s\n", pin, flg_str);
      to_binstr(AllMask, str, 32);
      printf("joy[%d]: mask(%s)\n", pin, str);
    }
  }
  else if (debug_on() >= DEBUG_GPIO) {
    printf("GPIO Configuration for GPIO%02d for %s skipped(%c)\n", pin, flg_str, GpioFlags[pin]);
  }

  joy_data[0].num_buttons = gpios_used();

  return(1);
}


void joy_RPi_poll(void)
{
  FILE* File;
  int Joystick;
  int Index;
  int Char;
  int newGpio;
  int iomask;

  Joystick = 0;

  /* get current GPIO pin states */
  newGpio = ((int*)GPIO)[GPIO_ADDR_OFFSET] & AllMask;

  //printf("%d: %08x\n", bounceCount, newGpio);
    
  if(newGpio != lastGpio){
    bounceCount=0;
    xGpio |= newGpio ^ lastGpio;
    //printf("%08x\n", xGpio);
  }
  lastGpio = newGpio;

  /* remove expanders from change monitor */
  xGpio &= ~joy_data[Joystick].xio_mask;

  if(bounceCount>=BOUNCE_TIME){
    joy_data[Joystick].button_mask = newGpio;
    joy_data[Joystick].change_mask = xGpio;

    for (Index = 0; Index < joy_data[Joystick].num_buttons; ++Index){
      iomask = (1 << gpio_pin(Index));
      joy_data[Joystick].buttons[Index] = !(newGpio & iomask);
      joy_data[Joystick].change[Index] = !!(xGpio & iomask);
    }
    xGpio = 0;
  }

  if(bounceCount<BOUNCE_TIME)bounceCount++;

  joy_handle_event();
}

void joy_enable_repeat(void)
{
  doRepeat = 1;
}

static void joy_handle_repeat(void)
{
  const struct {
    int time[4];
    int value[4];
    int next[4];
  }mxkey = {
    {80, 200, 40, 40},
    {0, 1, 0, 1},
    {1, 2, 3, 2}
  };
  /* key repeat metrics: release after 80ms, press after 200ms, release after 40ms, press after 40ms */

  static int idx = -1;
  static int prev_key = -1;
  static unsigned t_now = 0;
  static unsigned t_next = 0;
  keyinfo_s ks;

  get_last_key(&ks);

  if(doRepeat){
    if(!ks.val || (ks.key != prev_key)){ /* restart on release or key change */
      prev_key = ks.key;
      idx=-1;
      t_next = t_now;
    }
    else if(idx<0){ /* start new cycle */
      idx = 0;
      t_next = t_now + mxkey.time[idx];
    }
    else if(t_now == t_next){
      sendKey(ks.key, mxkey.value[idx]);
      idx = mxkey.next[idx];
      t_next = t_now + mxkey.time[idx];
    }
    t_now+=4; /* runs every 4 ms */
  }
}

void joy_handle_event(void)
{
  int Joystick = 0;
  int Index;

  /* handle all active irqs */
  if(~joy_data[Joystick].button_mask & joy_data[Joystick].xio_mask){ /* if active ints exist */
    //printf("XIO = %08x\n", ~joy_data[Joystick].button_mask & joy_data[Joystick].xio_mask);
    for (Index = 0; Index < joy_data[Joystick].num_buttons; ++Index){
      if( joy_data[Joystick].is_xio[Index] & joy_data[Joystick].buttons[Index]){
	send_gpio_keys(gpio_pin(Index), joy_data[Joystick].buttons[Index]);
      } 
    }
  }
  /* handle normal gpios */
  if(joy_data[Joystick].change_mask){
    //printf("GPIOs = %08x\n", joy_data[Joystick].button_mask);
    joy_data[Joystick].change_mask = 0;
    for (Index = 0; Index < joy_data[Joystick].num_buttons; ++Index){
      if( joy_data[Joystick].change[Index] ){
	joy_data[Joystick].change[Index] = 0;
	//printf("Button %d = %d\n", Index, joy_data[Joystick].buttons[Index]);
	send_gpio_keys(gpio_pin(Index), joy_data[Joystick].buttons[Index]);
      } 
    }
  }
  joy_handle_repeat();
}


