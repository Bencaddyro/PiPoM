#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>    // read/write usleep
#include <stdlib.h>    // exit function
#include <inttypes.h>  // uint8_t, etc
#include <linux/i2c-dev.h> // I2C bus definitions
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

#include "ads1115.h"


uint8_t running = 1;

void handle_sigUSR(int sig){
  running = 0;
}


void setup(int ads_addr, int *addr_fd){
  int fd;
  // open device on /dev/i2c-1 the default on Raspberry Pi B
  if ((fd = open("/dev/i2c-1", O_RDWR)) < 0) {
    printf("Error: Couldn't open device! %d\n", fd);
    exit (1);
  } 
  // connect to ADS1115 as i2c slave
  if (ioctl(fd, I2C_SLAVE, ads_addr) < 0) {
    printf("Error: Couldn't find device on address!\n");
    exit (1);
  }
  *addr_fd = fd;
}


void writeConf(int ads_fd, conf_ADS conf){
  uint8_t writeBuf[3];

  writeBuf[0] = 1; //conf register ADDR
  writeBuf[1] =
    conf.status << 7 |   //start conversion or probe status
    conf.mode_IO << 4 |  //choose I/O to measure
    conf.range_V << 1 |  //choose V range
    conf.mode;           //oneshot/continuous mode
  writeBuf[2] =
    conf.SPS << 5 |      //choose SPS
    0b00011;             //we do not implement comparator or alert function
  
  if (write(ads_fd, writeBuf, 3) != 3) {
    perror("Write to register 1");
    exit (1);
  }
}

int16_t readValue(int ads_fd){
  uint8_t readBuf[2];
  
  readBuf[0] = 0;
  if (write(ads_fd, readBuf, 1) != 1) {
    perror("Write register select");
    exit(-1);
  }
  // read conversion register
  if (read(ads_fd, readBuf, 2) != 2) {
    perror("Read conversion");
    exit(-1);
  }
  
  return readBuf[0] << 8 | readBuf[1];
}

void powerLog(conf_ADS cfg_tension, conf_ADS cfg_current, FILE *log_fd, int net_fd, char output, char verbose, int ads_fd){

  double tension_Rasp, tension_Shunt;
  int16_t tension_16, current_16;

  char *log;
  time_t second = time(NULL);
  int u_second = 0;
  int flag = MSG_MORE;

  while(running){

    // Timestamp information & network buffer
    if(second != time(NULL)){
      second = time(NULL);
      u_second = 0;         //reset millisec
      flag = 0;             //Network aggregation = 1s
    }
    else{
      u_second = u_second + cfg_tension.step_SPS + cfg_current.step_SPS;
      flag = MSG_MORE;
    }


    //printf("sampling %u \n",cfg_tension.SPS);
    //printf("sampling step %u \n",cfg_tension.step_SPS);

    //Measure tension

    writeConf(ads_fd, cfg_tension);   //Launch measure
    usleep(cfg_tension.step_SPS);     //Wait
    tension_16 = readValue(ads_fd);   //Read measure

    //Measure current (shunt tension)

    writeConf(ads_fd, cfg_current);   //Launch measure
    usleep(cfg_current.step_SPS);     //Wait
    current_16 = readValue(ads_fd);   //Read measure

    tension_Rasp = tension_16 * cfg_tension.step_V;
    tension_Shunt = current_16 * cfg_current.step_V;

    asprintf(&log, "%12ld;%10d;%10f;%10f;\n", second, u_second, tension_Rasp, tension_Shunt);

    if(output)     //Log in standard output
      printf(log);

    if(log_fd)    //Log in local file
      fprintf(log_fd, log);

    if(net_fd)    //Log in network
      send(net_fd, log, strlen(log), flag);    
  }
  
}
