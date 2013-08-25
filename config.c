/**** config.c *****************************/
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
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "config.h"
#include "iic.h"
#include "joy_RPi.h"

#define NUM_GPIO 32
#define MAX_LN 128
#define MAX_XIO_DEVS 8
#define MAX_XIO_PINS 8

extern key_names_s key_names[];

typedef struct _gpio_key{
  int gpio;
  int idx;
  int key;
  int xio; /* -1 for direct gpio */
  struct _gpio_key *next;
}gpio_key_s;

/* each I/O expander can have 8 pins */
typedef struct{
  char name[20];
  iodev_e type;
  int addr;
  int regno;
  int inmask;
  int lastvalue;
  gpio_key_s *last_key;
  gpio_key_s *key[8];
}xio_dev_s;

static int load_buffer(int fd);
static char *next_token(int fd);
static int next_command(int fd, char ***cmd);
static void parse_err(char *str);
static int find_key(const char *name);
static void add_event(gpio_key_s **ev, int gpio, int key, int xio);
static gpio_key_s *get_event(gpio_key_s *ev, int idx);
static int find_xio(const char *name);
static void setup_xio(int xio);

static gpio_key_s *gpio_key[NUM_GPIO];
static gpio_key_s *last_gpio_key = NULL;
static int gpios[NUM_GPIO];
static int num_gpios_used=0;
static xio_dev_s xio_dev[MAX_XIO_DEVS];
static int xio_count = 0;

static int SP;
static keyinfo_s KI;

static char *parse_buf = (char *) 0;
static char *parse_bufptr = (char *) 0;
static char parse_filename[80];
static int parse_lnno = 0;

int init_config(void)
{
  int i, j, k, n;
  int fd;
  char *end_ptr;
  char **cmd;
  int tok_cnt;
  char xname[32];
  int gpio, caddr, regno;
  char err_str[80];

  for(i=0;i<NUM_GPIO;i++){
    gpio_key[i] = NULL;
  }

  /* search for conf file: ./pikeyd.conf, ~/.pikeyd.conf, /etc/pikeyd.conf */
  strcpy(parse_filename, "./pikeyd.conf");
  fd = open(parse_filename, O_RDONLY);
  if (fd==-1) {
    sprintf(parse_filename, "%s/.pikeyd.conf", getenv("HOME"));
    fd = open(parse_filename, O_RDONLY);
    if (fd == -1) {
      strcpy(parse_filename, "/etc/pikeyd.conf");
      fd = open(parse_filename, O_RDONLY);
      if (fd == -1) {
        perror(parse_filename);
        perror("/etc/pikeyd.conf");
        return 0;
      }
    }
  }
  printf("Config file is %s\n", parse_filename);
  load_buffer(fd);

  /*
  while ((tok_cnt = next_command(fd, &cmd)) >= 0) {
    int i;
    if (tok_cnt) {
      for(i=0;i<tok_cnt;i++) printf("%s[%s]",(i?",":"tok:"),(char *) cmd[i]);
      printf("\n");
    }
  }
  return(0);
  */

  while ((tok_cnt = next_command(fd, &cmd)) >= 0) {

    /* skip blank lines */
    if (!tok_cnt) {
      continue;
    }

    /* KEY declaration */
    if (strncmp(cmd[0], "KEY", 3) == 0) {

      /* verify our syntax */
      if (tok_cnt != 2) {
        sprintf(err_str, "\'KEY\' definition requires 1 value. (%d given)", tok_cnt-1);
        parse_err(err_str);
        return(0);
      }

      /* check it is a known KEY */
      if ((k = find_key(cmd[0])) == 0) {
        sprintf(err_str, "Unknown KEY value (%s)", cmd[0]);
        parse_err(err_str);
        return(0);
      }

      /* test for I/O Expander definition */
      if (strncmp(cmd[1], "XIO", 3) == 0) {
        if (sscanf(cmd[1], "%[^:]:%i", xname, &gpio) == 2) {
          //printf("%d XIO event %s at (%s):%d\n", lnno, name, xname, gpio);
          if( (n = find_xio(xname)) >= 0 ){
            k = find_key(cmd[0]);
            if(k){
              add_event(&xio_dev[n].key[gpio], gpio, key_names[k].code, -1);
              //printf(" Added event %s on %s:%d\n", name, xname, gpio);
            }
          }
          else {
            sprintf(err_str, "Unknown expander: %s", xname);
            parse_err(err_str);
            return(0);
          }
        }
        else {
          sprintf(err_str, "Invalid expander definition: %s", cmd[1]);
          parse_err(err_str);
          return(0);
        }
      }
      /* Otherwise we assume it is a direct pin definition */
      else {
        gpio = (int) strtol(cmd[1], &end_ptr, 10);
        if (!*end_ptr) {

          if(key_names[k].code < 0x300){
            SP=0;
            add_event(&gpio_key[gpio], gpio, key_names[k].code, -1);
          }
        }
        else {
          sprintf(err_str, "Unknown KEY definition (%s)", cmd[1]);
          parse_err(err_str);
          return(0);
        }
      }
    }
    /* expander declarations start with "XIO" */
    else if (strncmp(cmd[0], "XIO", 3) == 0) {

      /* verify our syntax */
      if (tok_cnt != 2) {
        sprintf(err_str, "\'XIO\' expander definition requires 1 value. (%d given)", tok_cnt-1);
        parse_err(err_str);
        return(0);
      }

      n=sscanf(cmd[1], "%d/%i/%s", &gpio, &caddr, xname);
      if(n > 2){
        //printf("%d XIO entry: %s %d %02x %s\n",lnno,name,gpio,caddr,xname);

        strncpy(xio_dev[xio_count].name, cmd[0], 20);
        xio_dev[xio_count].addr = caddr;
        xio_dev[xio_count].last_key = NULL;
        xio_dev[xio_count].lastvalue = 0xff;
        for(i=0;i<8;i++){
          xio_dev[xio_count].key[i] = NULL;
        }
        if( !strncmp(xname, "MCP23008", 8) ){
          xio_dev[xio_count].type = IO_MCP23008;
          xio_dev[xio_count].regno = 0x09;
        }
        else if( !strncmp(xname, "MCP23017A", 9) ){
          xio_dev[xio_count].type = IO_MCP23017A;
          xio_dev[xio_count].regno = 0x09;
        }
        else if( !strncmp(xname, "MCP23017B", 9) ){
          xio_dev[xio_count].type = IO_MCP23017B;
          xio_dev[xio_count].regno = 0x19;
        }
        else{
          xio_dev[xio_count].type = IO_UNK;
          xio_dev[xio_count].regno = 0;
        }

        add_event(&gpio_key[gpio], gpio, 0, xio_count);
        xio_count++;
        xio_count %= 16;
      }
      else {
        sprintf(err_str, "Invalid XIO data for %s [%s]", cmd[0], cmd[1]);
        parse_err(err_str);
        return(0);
      }
    }
    else{
      sprintf(err_str, "Unknown configuration item: %s", cmd[0]);
      parse_err(err_str);
      return(0);
    }
  }

  close(fd);

  n=0;
  for(i=0; i<NUM_GPIO; i++){
    if(gpio_key[i]){
      gpios[n] = gpio_key[i]->gpio;
      n++;
    }
  }
  num_gpios_used = n;

  for(j=0;j<xio_count;j++){
    for(i=0;i<8;i++){
      if(xio_dev[j].key[i]){
	xio_dev[j].inmask |= 1<<i;
      }
    }
    setup_xio(j);
    test_iic(xio_dev[j].addr, xio_dev[j].regno & 0x10);
  }

  printf("Polling %d GPIO pin(s).\n", num_gpios_used);
  printf("Found %d I/O expander(s).\n", xio_count);
  printf("Ready.\n");

  return(xio_count?2:1);
}

static int load_buffer(int fd) {

  if (!parse_buf) {
    parse_buf = (char *) malloc(BUFSIZ+1);
  }
  memset( (char *) parse_buf, 0, BUFSIZ+1);
  parse_bufptr = parse_buf;

  return(read(fd, parse_buf, BUFSIZ));
}

static char *next_token(int fd) {

  const char whitespace[] = " \t\r#\0";
  char *tok = (char *) 0;

  /* chew though white space and comments to next token */
  while ((*parse_bufptr) &&
         (strchr(whitespace, (int) *parse_bufptr) != NULL)) {

    /* comments run until CR */
    if (*parse_bufptr == '#') {
      while ((*parse_bufptr) && (*parse_bufptr != '\n')) {
        parse_bufptr++;
        if (*parse_bufptr == (char) 0) {
          load_buffer(fd);
        }
      }
    }
    else {
      parse_bufptr++;
      if (*parse_bufptr == (char) 0) {
        load_buffer(fd);
      }
    }
  }

  /* was this an empty line */
  if (*parse_bufptr == '\n') {
    tok = strdup("\n");
    parse_bufptr++;
    parse_lnno++;
  }
  else {

    /* gobble up the token */
    while ((*parse_bufptr) &&
           (strchr(whitespace, (int) *parse_bufptr) == NULL) &&
           (*parse_bufptr != '\n')) {

      if (tok == (char *) 0) {
        tok = (char *) malloc(2);
        memset( (void *) tok, 0, 2);
        *tok = *parse_bufptr;
      }
    else {
        tok = (char *) realloc( (void *) tok, strlen(tok)+2);
        tok[strlen(tok)+1] = 0;
        tok[strlen(tok)] = *parse_bufptr;
      }
      parse_bufptr++;
      if (*parse_bufptr == (char) 0) {
        load_buffer(fd);
      }
    }
  }

  return(tok);
}

static int next_command(int fd, char ***cmd) {

  char *tok;
  int cnt = -1;

  *cmd = (char **) 0;
  while ((tok = next_token(fd)) && (*tok != '\n')) {

    if (!*cmd) {
      *cmd = (char **) calloc(1, sizeof(char *));
      (*cmd)[0] = tok;
      cnt = 1;
    }
    else {
      cnt++;
      *cmd = (char **) realloc((void *) *cmd, sizeof(char *)*cnt);
      (*cmd)[cnt-1] = tok;
    }
  }

  /* check if this is just a blank line */
  if ((cnt == -1) && tok) {
    cnt = 0;
  }

  return(cnt);
}

static void parse_err(char *str) {

  printf("ERROR: %s line %d: %s\n", parse_filename, parse_lnno, str);

  return;
}

static int find_xio(const char *name)
{
  int i=0;
  while(i<xio_count){
    if(!strncmp(xio_dev[i].name, name, 32))break;
    i++;
  }
  if(i >= xio_count){
    i = -1;
  }
  return i;
}

static int find_key(const char *name)
{
  int i=0;
  while(key_names[i].code >= 0){
    if(!strncmp(key_names[i].name, name, 32))break;
    i++;
  }
  if(key_names[i].code < 0){
    i = 0;
  }
  return i;
}

static void add_event(gpio_key_s **ev, int gpio, int key, int xio)
{
  if(*ev){
    SP++;
    /* Recursive call to add the next link in the list */
    add_event(&(*ev)->next, gpio, key, xio);
  }
  else{
    *ev = malloc(sizeof(gpio_key_s));
    if(*ev==NULL){
      perror("malloc");
    }
    else{
      (*ev)->gpio = gpio;
      (*ev)->idx = SP;
      (*ev)->key = key;
      (*ev)->xio = xio;
      (*ev)->next = NULL;
    }
  }
}

static gpio_key_s *get_event(gpio_key_s *ev, int idx)
{
  if(ev->idx == idx){
    return ev;
  }
  else if(!ev->next){
    return NULL;
  }
  else{
    return get_event(ev->next, idx);
  }
}

int get_event_key(int gpio, int idx)
{
  gpio_key_s *ev;
  ev = get_event(gpio_key[gpio], idx);
  if(ev){
    return ev->key;
  }
  else{
    return 0;
  }
}

int got_more_keys(int gpio)
{
  if( last_gpio_key == NULL ){
    return (gpio_key[gpio] != NULL);
  }
  else if( last_gpio_key->next == NULL){
    return 0;
  }
  else{
    return 1;
  }
}

void restart_keys(void)
{
    last_gpio_key = NULL;
}

int get_curr_key(void)
{
  int r=0;
  if(last_gpio_key){
    r=last_gpio_key->key;
  }
  return r;
}

int get_next_key(int gpio)
{
  static int lastgpio=-1;
  static int idx = 0;
  gpio_key_s *ev;
  int k;

  ev = last_gpio_key;
  if( (ev == NULL) || (gpio != lastgpio) ){
    /* restart at the beginning after reaching the end, or reading a new gpio */
    ev = gpio_key[gpio];
    lastgpio = gpio;
  }
  else{
    /* get successive events while retrieving the same gpio */
    ev = last_gpio_key->next;
  }
  last_gpio_key = ev;

  if(ev){
    k = ev->key;
  }
  else{
    k = 0;
  }
  return k;
}

void test_config(void)
{
  int e, i, k;
  e=21;

  for(i=0; i<NUM_GPIO; i++){
    if(gpio_key[i]){
      printf(" %d\n", i);
    }
  }

  while( got_more_keys(e) ){
    k = get_next_key(e);
    printf("%d: EV %d = %d\n", i++, e, k);
    if(i>8)break;
  }
  i=0;
  restart_keys();
  while( got_more_keys(e) ){
    k = get_next_key(e);
    printf("%d: EV %d = %d\n", i++, e, k);
    if(i>8)break;
  }
}

int gpios_used(void)
{
  return num_gpios_used;
}

int gpio_pin(int n)
{
  return gpios[n % NUM_GPIO];
}

int is_xio(int gpio)
{
  int r=0;
  if( gpio_key[gpio] && (gpio_key[gpio]->xio >= 0) ){
    r=1;
  }
  return r;
}

static void setup_xio(int xio)
{
  char cfg_dat[]={
    0xff, //IODIR
    0x00, //IPOL
    xio_dev[xio].inmask,  //GPINTEN - enable interrupts for defined pins
    0x00, //DEFVAL
    0x00, //INTCON - monitor changes
    0x84, //IOCON - interrupt pin is open collector;
    0xff, //GPPU - enable all pull-ups
  };
  char buf[]={0x84};
  int addr = xio_dev[xio].addr;

  /* first ensure that the bank bit is set */
  if( write_iic(addr, 0x0a, buf, 1) < 0 ){
    perror("iic init write 1\n");
  }
  buf[0]=0; /* reset OLATA if incorrectly addressed before */
  if( write_iic(addr, 0x0a, buf, 1) < 0 ){
    perror("iic init write 2\n");
  }

  switch(xio_dev[xio].type){
  case IO_MCP23008:
    write_iic(addr, 0, cfg_dat, 7); 
    printf("Configuring MCP23008\n");
    break;
  case IO_MCP23017A:
    write_iic(addr, 0, cfg_dat, 7); 
    printf("Configuring MCP23017 port A\n");
    break;
  case IO_MCP23017B:
    write_iic(addr, 0x10, cfg_dat, 7); 
    printf("Configuring MCP23017 port B\n");
    break;
  default:
    break;
  }
}

int get_curr_xio_no(void)
{
  int n;
  int r = -1;
  if(last_gpio_key){
    n=last_gpio_key->xio;
    if(n>=0){
      r = n;
    }
  }
  return r;
}

void get_xio_parm(int xio, iodev_e *type, int *addr, int *regno)
{
  *type = xio_dev[xio].type;
  *addr = xio_dev[xio].addr;
  *regno = xio_dev[xio].regno;
}

int got_more_xio_keys(int xio, int gpio)
{
  if( xio_dev[xio].last_key == NULL ){
    return (xio_dev[xio].key[gpio] != NULL);
  }
  else if( xio_dev[xio].last_key->next == NULL){
    return 0;
  }
  else{
    return 1;
  }
}

int get_next_xio_key(int xio, int gpio)
{
  static int lastgpio=-1;
  static int idx = 0;
  gpio_key_s *ev;
  int k;

  ev = xio_dev[xio].last_key;
  if( (ev == NULL) || (gpio != lastgpio) ){
    /* restart at the beginning after reaching the end, or reading a new gpio */
    ev = xio_dev[xio].key[gpio];
    lastgpio = gpio;
  }
  else{
    /* get successive events while retrieving the same gpio */
    ev = xio_dev[xio].last_key->next;
  }
  xio_dev[xio].last_key = ev;

  if(ev){
    k = ev->key;
  }
  else{
    k = 0;
  }
  return k;
}

void restart_xio_keys(int xio)
{
    xio_dev[xio].last_key = NULL;
}

void handle_iic_event(int xio, int value)
{
  int ival = value & xio_dev[xio].inmask;
  int xval = ival ^ xio_dev[xio].lastvalue;
  int f,i,k=0,x;

  xio_dev[xio].lastvalue = ival;

  for (i = 0; i < MAX_XIO_PINS; i++){
    restart_xio_keys(xio);
    while(got_more_xio_keys(xio, i)){
      k = get_next_xio_key(xio, i);
      x = !(ival & (1 << i)); /* is the pin high or low? */
      f = xval & (1 << i); /* has the pin changed? */
      if(f){
	//printf("(%02x) sending %d/%d: %x=%d\n", ival, xio, i, k, x);
	sendKey(k, x); /* switch is active low */
	KI.key = k; KI.val = x;
	if(x && got_more_xio_keys(xio, i)){
	  /* release the current key, so the next one can be pressed */
	  //printf("sending %x=%d\n", k,x);
	  sendKey(k, 0);
	}
      }
    }
    //printf("Next pin\n");
  }
  //printf("handler exit\n");
}

void last_iic_key(keyinfo_s *kp)
{
  kp->key = KI.key;
  kp->val = KI.val;
}

