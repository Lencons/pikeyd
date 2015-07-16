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
#include "gpio.h"
#include "debug.h"

#define NUM_P1_PINS 26
#define NUM_P5_PINS 8
#define MAX_LN 128
#define MAX_XIO_DEVS 8
#define MAX_XIO_PINS 8

extern key_names_s key_names[];

/* each I/O expander can have 8 pins */
typedef struct{
  char *name;
  iodev_e type;
  int addr;
  int regno;
  int inmask;
  int lastvalue;
  gpio_key_s *last_key;
  gpio_key_s *key[8];
}xio_dev_s;

<<<<<<< HEAD
static int load_buffer(int fd);
static char *next_token(int fd);
static int next_command(int fd, char ***cmd);
static void parse_err(char *str);
static int find_key(const char *name);
static void add_event(gpio_key_s **ev, int gpio, int key, int xio);
static gpio_key_s *get_event(gpio_key_s *ev, int idx);
static int find_xio(const char *name);
static void setup_xio(int xio);
=======
int load_buffer(int fd);
char *next_token(int fd);
int next_command(int fd, char ***cmd);
void parse_err(char *str);
int parse_pin_str(char *str, int *val[]);
void test_config(void);
int get_gpio_pin(char *pin_str);
int find_key(const char *name);
int get_pin_ref(char *str, int *gpio, int *mat_grp, int *xio);
void add_event(gpio_key_s **ev, int gpio, int key, int xio);
gpio_key_s *get_event(gpio_key_s *ev, int idx);
int find_xio(const char *name);
void setup_xio(int xio);
int find_mat(char *name);
>>>>>>> key_matrix

static int gpios[NUM_GPIO];
static xio_dev_s xio_dev[MAX_XIO_DEVS];
static int xio_count = 0;

static mat_grp_s *mat_grp;
static int mat_cnt = 0;

static int SP;
static keyinfo_s KI;

<<<<<<< HEAD
=======
/* config file parsing variables */
>>>>>>> key_matrix
static char *parse_buf = (char *) 0;
static char *parse_bufptr = (char *) 0;
static char parse_filename[80];
static int parse_lnno = 0;

<<<<<<< HEAD
int init_config(void)
{
  int i, j, k, n;
  int fd;
=======

/* init_config() should be called once to read configuration from file.
 * The GPIO will also be configured as part of this process and should
 * already be initalised.
 */
int init_config(void) {

  int i, j, k, n;
  int fd;
  int grp_id, xio;
>>>>>>> key_matrix
  char *end_ptr;
  char **cmd;
  int tok_cnt;
  char xname[32];
  int gpio, caddr, regno;
  char err_str[80];

<<<<<<< HEAD
  for(i=0;i<NUM_GPIO;i++){
    gpio_key[i] = NULL;
  }
=======
  /* initalise default matrix group for direct I/O */
  mat_grp = (mat_grp_s *) malloc(sizeof(mat_grp_s));
  memset((void *) mat_grp, 0, sizeof(mat_grp_s));
  mat_cnt = 0;
  mat_grp[0].gpio = -1;
>>>>>>> key_matrix

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

<<<<<<< HEAD
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

=======
  /* process the configuration file */
>>>>>>> key_matrix
  while ((tok_cnt = next_command(fd, &cmd)) >= 0) {

    /* skip blank lines */
    if (!tok_cnt) {
      continue;
    }

<<<<<<< HEAD
    /* KEY declaration */
=======
    /**
     ** KEY_ declaration
     ** ===============
     **/
>>>>>>> key_matrix
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

<<<<<<< HEAD
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
=======
      switch(get_pin_ref(cmd[1], &gpio, &grp_id, &xio)) {
        case 0:
          printf("KEY Configuration - Ineternal Error\n");
          return(0);
        case -1:
          sprintf(err_str, "Unknown expander: %s", cmd[1]);
          parse_err(err_str);
          return(0);
        case -2:
          sprintf(err_str, "Invalid expander definition: %s", cmd[1]);
          parse_err(err_str);
          return(0);
        case -3:
          sprintf(err_str, "Matrix group not defined (%s)", cmd[1]);
          parse_err(err_str);
          return(0);
        case -4:
          sprintf(err_str, "Invalid GPIO PIN reference (%s)", cmd[1]);
          parse_err(err_str);
          return(0);
        case -5:
          sprintf(err_str, "Invalid Matrix definition: %s", cmd[1]);
          parse_err(err_str);
          return(0);
        case -6:
          sprintf(err_str, "Invalid GPIO PIN reference (%s)", cmd[1]);
          parse_err(err_str);
          return(0);
      }

      if (debug_lvl() >= DEBUG_DEV1) {
        printf("REPEAT: %s GPIO: %d MATRIX: %d XIO: %d\n", cmd[1], gpio, grp_id, xio);
      }

      if (grp_id >= 0) {
        switch (gpio_pincfg(gpio, GPIO_IN, &mat_grp[grp_id].gpio_mask)) {
          case 0:
            sprintf(err_str, "INTERNAL ERROR: gpio_pincfg()");
            parse_err(err_str);
            return(0);
          case -1:
            sprintf(err_str, "GPIO%0d already configured for output.", gpio);
            parse_err(err_str);
            return(0);
        }
        mat_grp[grp_id].last_gpio = mat_grp[grp_id].gpio_mask;
        add_event(&(mat_grp[grp_id].gpio_key[gpio]), gpio, key_names[k].code, -1);
      }
      else if (xio >= 0) {
        add_event(&xio_dev[xio].key[gpio], gpio, key_names[k].code, -1);
        if (debug_lvl() >= DEBUG_DEV1) {
          printf(" Added event %s on %s:%d\n", key_names[k].code, xname, gpio);
        }
      }
      else {
        if (key_names[k].code < 0x300) {
          SP=0;
          switch (gpio_pincfg(gpio, GPIO_IN, &mat_grp[0].gpio_mask)) {
            case 0:
              sprintf(err_str, "INTERNAL ERROR: gpio_pincfg()");
              parse_err(err_str);
              return(0);
            case -1:
              sprintf(err_str, "GPIO%0d already configured for output.", gpio);
              parse_err(err_str);
              return(0);
          }
          mat_grp[0].last_gpio = mat_grp[0].gpio_mask;
          add_event(&(mat_grp[0].gpio_key[gpio]), gpio, key_names[k].code, -1);
        }
      }
    }

    /**
     ** XIO expander declaration
     ** ========================
     **/
>>>>>>> key_matrix
    else if (strncmp(cmd[0], "XIO", 3) == 0) {

      /* verify our syntax */
      if (tok_cnt != 2) {
        sprintf(err_str, "\'XIO\' expander definition requires 1 value. (%d given)", tok_cnt-1);
        parse_err(err_str);
        return(0);
      }

<<<<<<< HEAD
      n=sscanf(cmd[1], "%d/%i/%s", &gpio, &caddr, xname);
      if(n > 2){
        //printf("%d XIO entry: %s %d %02x %s\n",lnno,name,gpio,caddr,xname);

        strncpy(xio_dev[xio_count].name, cmd[0], 20);
=======
      /* check this isn't a duplicate entry */
      if (find_xio(cmd[0]) != -1) {
        sprintf(err_str, "Duplicate \'XIO\' expander definition: %s", cmd[0]);
        parse_err(err_str);
        return(0);
      }

      n=sscanf(cmd[1], "%d/%i/%s", &gpio, &caddr, xname);
      if(n == 3){
        //printf("%d XIO entry: %s %d %02x %s\n",lnno,name,gpio,caddr,xname);

        xio_dev[xio_count].name = strdup(cmd[0]);
>>>>>>> key_matrix
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

<<<<<<< HEAD
        add_event(&gpio_key[gpio], gpio, 0, xio_count);
        xio_count++;
        xio_count %= 16;
=======
        add_event(&(mat_grp[0].gpio_key[gpio]), gpio, 0, xio_count);
        xio_count++;
        xio_count %= MAX_XIO_DEVS;
>>>>>>> key_matrix
      }
      else {
        sprintf(err_str, "Invalid XIO data for %s [%s]", cmd[0], cmd[1]);
        parse_err(err_str);
        return(0);
      }
    }
<<<<<<< HEAD
    else{
=======

    /**
     ** MATRIX group definition
     ** =======================
     **/
    else if (strncmp(cmd[0], "MATRIX", 6) == 0) {

      /* check this isn't a duplicate entry */
      if (find_mat(cmd[0]) != -1) {
        sprintf(err_str, "Duplicate \'MATIX\' group definition: %s", cmd[0]);
        parse_err(err_str);
        return(0);
      }

      /* verify our syntax */
      if (tok_cnt != 2) {
        sprintf(err_str, "\'MATRIX\' group definition requires 1 value. (%d given)", tok_cnt-1);
        parse_err(err_str);
        return(0);
      }

      gpio = get_gpio_pin(cmd[1]);
      if (gpio >= 0) {
        mat_cnt++;
        mat_grp = (mat_grp_s *) realloc((void *) mat_grp, sizeof(mat_grp_s) * (mat_cnt + 1));
        memset((void *) &mat_grp[mat_cnt], 0, sizeof(mat_grp_s));
        mat_grp[mat_cnt].name = strdup(cmd[0]);
        mat_grp[mat_cnt].gpio = gpio;

        switch (gpio_pincfg(gpio, GPIO_OUT, (int *) 0)) {
          case 2:
            sprintf(err_str, "Matrix driver GPIO%02d already configured.", gpio);
            parse_err(err_str);
            return(0);
          case 0:
            sprintf(err_str, "INTERNAL ERROR: gpio_pincfg()");
            parse_err(err_str);
            return(0);
          case -1:
            sprintf(err_str, "GPIO%0d already configured for output.", gpio);
            parse_err(err_str);
            return(0);
        }
      }
      else {
        sprintf(err_str, "Invalid GPIO PIN reference (%s)", cmd[1]);
        parse_err(err_str);
        return(0);
      }
    }

    /**
     ** PULL Internal Pull Resistor configuration
     ** ==========================================
     **/
    else if (strncmp(cmd[0], "PULL", 4) == 0) {

      int mode;

      /* verify our syntax */
      if (tok_cnt != 2) {
        sprintf(err_str, "\'PULL\' definition requires 1 value. (%d given)", tok_cnt-1);
        parse_err(err_str);
        return(0);
      }

      if (!strcmp(cmd[0], "PULL_DOWN")) {
        mode = PUD_DOWN;
      }
      else if (!strcmp(cmd[0], "PULL_UP")) {
        mode = PUD_UP;
      }
      else if (!strcmp(cmd[0], "PULL_FLOAT")) {
        mode = PUD_OFF;
      }
      else {
        sprintf(err_str, "Invalid PULL command (%s)", cmd[0]);
        parse_err(err_str);
        return(0);
      }

      gpio = get_gpio_pin(cmd[1]);
      if (gpio >= 0) {
        if (!gpio_pull(gpio, mode)) {
          sprintf(err_str, "GPIO%02d pull resister not set, pin no set for input.", gpio);
          parse_err(err_str);
          return(0);
        }
      }
      else {
        sprintf(err_str, "Invalid GPIO PIN reference (%s)", cmd[1]);
        parse_err(err_str);
        return(0);
      }
    }

    /**
     ** REPEAT management for keys
     ** ==========================
     **/
    else if (strncmp(cmd[0], "REPEAT", 6) == 0) {

      char *p, *q;

      /* verify our syntax */
      if (tok_cnt != 2) {
        sprintf(err_str, "\'REPEAT\' definition requires 1 value. (%d given)", tok_cnt-1);
        parse_err(err_str);
        return(0);
      }

      q = cmd[1];
      while (*q) {
        p = q;
        for (; *q && (*q != ','); q++);
        if (*q) {
          *q = (char) 0;
          q++;
        }

        switch(get_pin_ref(p, &gpio, &grp_id, &xio)) {
          case 0:
            printf("REPEAT Configuration - Ineternal Error\n");
            return(0);
          case -1:
            sprintf(err_str, "Unknown expander: %s", p);
            parse_err(err_str);
            return(0);
          case -2:
            sprintf(err_str, "Invalid expander definition: %s", p);
            parse_err(err_str);
            return(0);
          case -3:
            sprintf(err_str, "Matrix group not defined (%s)", p);
            parse_err(err_str);
            return(0);
          case -4:
            sprintf(err_str, "Invalid GPIO PIN reference (%s)", p);
            parse_err(err_str);
            return(0);
          case -5:
            sprintf(err_str, "Invalid Matrix definition: %s", p);
            parse_err(err_str);
            return(0);
          case -6:
            sprintf(err_str, "Invalid GPIO PIN reference (%s)", p);
            parse_err(err_str);
            return(0);
        }

        if (debug_lvl() >= DEBUG_DEV1) {
          printf("REPEAT: %s GPIO: %d MATRIX: %d XIO: %d\n", p, gpio, grp_id, xio);
        }

        if (grp_id) {
          mat_grp[grp_id].rpt_flg |= (1<<gpio);
        }
        else if (xio >= 0) {
          printf("Repeat not imlemented for XIO.\n");
        }
        else {
          mat_grp[0].rpt_flg |= (1<<gpio);
        }
      }
    }
    else {
>>>>>>> key_matrix
      sprintf(err_str, "Unknown configuration item: %s", cmd[0]);
      parse_err(err_str);
      return(0);
    }
<<<<<<< HEAD
=======

    /* clean up memory allocated at token parsing */
    for (i=0;i<tok_cnt;i++) {
      free(cmd[i]);
    }
    free(cmd);
>>>>>>> key_matrix
  }

  close(fd);

  n=0;
  for(i=0; i<NUM_GPIO; i++){
    if(mat_grp[0].gpio_key[i]){
      gpios[n] = mat_grp[0].gpio_key[i]->gpio;
      n++;
    }
  }

  for(j=0;j<xio_count;j++){
    for(i=0;i<8;i++){
      if(xio_dev[j].key[i]){
	xio_dev[j].inmask |= 1<<i;
      }
    }
    setup_xio(j);
    test_iic(xio_dev[j].addr, xio_dev[j].regno & 0x10);
  }

  if (debug_on()) {
    test_config();
  }

<<<<<<< HEAD
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
=======
  if (xio_count) {
    return(2);
  }
  if (mat_cnt) {
    return(3);
  }

  return(1);
>>>>>>> key_matrix
}

int load_buffer(int fd) {

  if (!parse_buf) {
    parse_buf = (char *) malloc(BUFSIZ+1);
  }
  memset( (char *) parse_buf, 0, BUFSIZ+1);
  parse_bufptr = parse_buf;

  return(read(fd, parse_buf, BUFSIZ));
}

char *next_token(int fd) {

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

int next_command(int fd, char ***cmd) {

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

void parse_err(char *str) {

  printf("ERROR: %s line %d: %s\n", parse_filename, parse_lnno, str);

  return;
}


int get_gpio_pin(char *pin_str) {

  static p1_ref[NUM_P1_PINS] = {-1,-1,2,-1,3,-1,4,14,-1,15,17,18,27,
                                -1,22,23,-1,24,10,-1,9,25,11,8,-1,7};
  static p5_ref[NUM_P5_PINS] = {-1,-1,28,29,30,31,-1,-1};

  int pin = -1;
  char *end_ptr;

  /* if a GPIO reference, just return the pin number */
  if (!strncmp(pin_str, "GPIO", 4)) {

    pin = (int) strtol(&pin_str[4], &end_ptr, 10);
    if (*end_ptr) {
      pin = -1;
    }
  }
  /* PIN declarations are GPIO connector pin numbers */
  else if (!strncmp(pin_str, "PIN", 3) || !strncmp(pin_str, "P1-", 3)) {
    int i;

    i = (int) strtol(&pin_str[3], &end_ptr, 10);
    if (!*end_ptr && (i >= 1) && (i <= NUM_P1_PINS)) {
      pin = p1_ref[i-1];
    }
  }
  /* connector pins on the P5 interface */
  else if (!strncmp(pin_str, "P5-", 3)) {
    int i;

    i = (int) strtol(&pin_str[3], &end_ptr, 10);
    if (!*end_ptr && (i >= 1) && (i <= NUM_P5_PINS)) {
      pin = p5_ref[i-1];
    }
  }
  /* straight GPIO pin value */
  else {
    pin = (int) strtol(pin_str, &end_ptr, 10);
    if (*end_ptr) {
      pin = -1;
    }
  }

  return(pin);
}

int find_key(const char *name)
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


int get_pin_ref(char *str, int *gpio, int *mat_grp, int *xio) {

  int n;
  char s[80];

  *gpio = -1;
  *mat_grp = -1;
  *xio = -1;

  /* test for I/O Expander definition */
  if (!strncmp(str, "XIO", 3)) {
    if (sscanf(str, "%[^:]:%i", s, gpio) == 2) {
      if( (*xio = find_xio(s)) < 0 ){
        /* Unknown expander */
        return(-1);
      }
    }
    else {
      /* Invalid expander definition */
      return(-2);
    }
  }

  /* test for MATRIX Pin definition */
  else if (!strncmp(str, "MATRIX", 6)) {
    char mat_str[80];

    if (sscanf(str, "%[^:]:%s", mat_str, s) == 2) {

      *mat_grp = find_mat(mat_str);
      if (*mat_grp == -1) {
        /* Matrix group not defined */
        return(-3);
      }

      *gpio = get_gpio_pin(s);
      if (*gpio < 0) {
        /* Invalid GPIO PIN reference */
        return(-4);
      }
    }
    else {
      /* Invalid Matrix definition */
      return(-5);
    }
  }
  /* Otherwise we assume it is a direct pin definition */
  else {
    *gpio = get_gpio_pin(str);
    if (gpio < 0) {
      /* Invalid GPIO PIN reference */
      return(-6);
    }
  }

  return(1);
}


void add_event(gpio_key_s **ev, int gpio, int key, int xio)
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

gpio_key_s *get_event(gpio_key_s *ev, int idx)
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
  ev = get_event(mat_grp[0].gpio_key[gpio], idx);
  if(ev){
    return ev->key;
  }
  else{
    return 0;
  }
}

int got_more_keys(int grp, int gpio)
{
  if( mat_grp[grp].last_gpio_key == NULL ){
    return (mat_grp[grp].gpio_key[gpio] != NULL);
  }
  else if( mat_grp[grp].last_gpio_key->next == NULL){
    return 0;
  }
  else{
    return 1;
  }
}

void restart_keys(int grp)
{
    mat_grp[grp].last_gpio_key = NULL;
}

int get_curr_key(int grp)
{
  int r=0;
  if(mat_grp[grp].last_gpio_key){
    r=mat_grp[grp].last_gpio_key->key;
  }
  return r;
}

int get_next_key(int grp, int gpio)
{
  static int lastgpio=-1;
  static int idx = 0;
  gpio_key_s *ev;
  int k;

  ev = mat_grp[grp].last_gpio_key;
  if( (ev == NULL) || (gpio != lastgpio) ){
    /* restart at the beginning after reaching the end, or reading a new gpio */
    ev = mat_grp[grp].gpio_key[gpio];
    lastgpio = gpio;
  }
  else{
    /* get successive events while retrieving the same gpio */
    ev = mat_grp[grp].last_gpio_key->next;
  }
  mat_grp[grp].last_gpio_key = ev;

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
  int i, j, k;
  int grp;
  char str[30];

  printf("** Configuration Dump\n**\n");

  if (mat_grp[0].gpio_mask) {
    printf("** Direct Button Configuration:\n");
    printf("**\tGPIO: ");
    j = 0;
    for (i=0; i<NUM_GPIO; i++) {
      if(mat_grp[0].gpio_key[i]){
        printf("%s%02d", j?", ":"", i);
        j++;
        if(j && !(j%15)) {
          printf("\n**\t      ");
          j = 0;
        }
      }
    }
    printf("\n**\n");

    printf("** GPIO Key Assignments:\n");
    for (i=0; i<NUM_GPIO; i++) {
      j = 0;
      restart_keys(0);
      sprintf(str, "**  GPIO%02d: ", i);
      while (got_more_keys(0, i)) {
        int x;

        k = get_next_key(0, i);
        for (x=0; (key_names[x].code != -1) && (key_names[x].code != k); x++);
        printf("%s<%s>", (j?", ":str), &key_names[x].name[4]);
        j++;
      }
      if (j) {
        printf("\n");
      }
    }
    printf("**\n");
  }

  if (mat_cnt) {
    printf("** Matrix Groups:\n");
    for (grp=1; grp <= mat_cnt; grp++) {
      for (i=0; i<NUM_GPIO; i++) {
        j = 0;
        restart_keys(grp);
        sprintf(str, "**  %s(GPIO%02d): ", mat_grp[grp].name, i);
        while (got_more_keys(grp, i)) {
          int x;

          k = get_next_key(grp, i);
          for (x=0; (key_names[x].code != -1) && (key_names[x].code != k); x++);
          printf("%s<%s>", j?", ":str, &key_names[x].name[4]);
          j++;
        }
        if (j) {
          printf("\n");
        }
      }
    }
    printf("**\n");
  }

  return;
}

/**
 ** GPIO Management Routines
 **/

int gpio_pin(int n)
{
  return gpios[n % NUM_GPIO];
}

/**
 ** External I/O Management Routines
 **/

int find_xio(const char *name)
{
  int i=0;
  while(i<xio_count){
    if(!strcmp(xio_dev[i].name, name))break;
    i++;
  }
  if(i >= xio_count){
    i = -1;
  }
  return i;
}

int is_xio(int gpio)
{
  int r=0;
  if( mat_grp[0].gpio_key[gpio] && (mat_grp[0].gpio_key[gpio]->xio >= 0) ){
    r=1;
  }
  return r;
}

void setup_xio(int xio)
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
  if(mat_grp[0].last_gpio_key){
    n=mat_grp[0].last_gpio_key->xio;
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

/**
 ** I2C Management Routines
 **/

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
	sendKey(k, 1); /* switch is active low */
	KI.key = k; KI.val = x;
        sendKey(k, 0);
      }
    }
    //printf("Next pin\n");
  }
  //printf("handler exit\n");
}


/**
 ** Matrix Grgoups Management Routines
 **/

int find_mat(char *name) {

  int i = -1;

  if (mat_cnt) {
    for (i=1; (i <= mat_cnt) && strcmp(mat_grp[i].name, name); i++);
    if (i > mat_cnt) {
      i = -1;
    }
  }

  return(i);
}

mat_grp_s *get_matgrp(int grp) {

  return(&mat_grp[grp]);
}

<<<<<<< HEAD
=======
int mat_count(void) {
  return(mat_cnt);
}

>>>>>>> key_matrix
