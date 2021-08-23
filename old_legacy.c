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

#include "ads1115.h"




void continuous(int ads_fd, uint8_t modeIO, uint8_t rangeV, double stepV, uint8_t SPS, int t_SPS, FILE *log_fd, int net_fd, char verbose){
  uint8_t writeBuf[3];
  uint8_t readBuf[2];
  int16_t val;
  uint8_t log_file = (log_fd != NULL);
  uint8_t log_net = (net_fd != NULL);
  double tension;
  
  time_t clock;
  time_t memclock;
  int uclock;
  char timeframe;


  char buffer[1024]; 
  int lenmsg = 0;
  
  writeBuf[0] = 1;
  writeBuf[1] =
    0b0 << 7 |     //no effect
    modeIO << 4 |  //choose I/O
    rangeV << 1 |  //choose V range
    0b0;           //continuous mode
  writeBuf[2] =
    SPS << 5 |     //choose SPS
    0b00011;       //we do not implement comparator or alert function
  
  if (write(ads_fd, writeBuf, 3) != 3) {
    perror("Write to register 1");
    exit (1);
  }
  usleep(200000);

  readBuf[0] = 0;
  if (write(ads_fd, readBuf, 1) != 1) {
    perror("Write register select");
    exit(-1);
  }
  while(running){
    //creation timestamp
    clock = time(NULL);
    if(clock != memclock){//1 sec have passed
      memclock = clock;
      uclock = 0;//reset millisec
      timeframe = 1;
    }
    else{
      uclock = uclock + t_SPS;

    }

    // read conversion register
    if (read(ads_fd, readBuf, 2) != 2) {
      perror("Read conversion");
      exit(-1);
    }
    // could also multiply by 256 then add readBuf[1]
    val = readBuf[0] << 8 | readBuf[1];
    tension = val * stepV;


    if(verbose){
      printf("[%ld@%d] ADS1115 read : %d, U = %f V\n",memclock,uclock,val,tension);
    }
    if(log_file){//write on local logfile logs
      fprintf(log_fd, "%ld;%d;%d;%f;\n",memclock,uclock,val,tension);
    }
    if(log_net){//write on UDP buffer logs
      lenmsg = lenmsg + sprintf(buffer + lenmsg,"%ld;%d;%d;%f;\n",memclock,uclock,val,tension);  
    }
    if(timeframe){
      send(net_fd,buffer,lenmsg,0);
      lenmsg = 0;
      timeframe = 0;
    }
    
    usleep(t_SPS);
  }
  running = 1;
}

void oneshot(int ads_fd, uint8_t modeIO, uint8_t rangeV, double stepV, uint8_t SPS){
  uint8_t writeBuf[3];
  uint8_t readBuf[2];
  int16_t val;

  writeBuf[0] = 1;
  writeBuf[1] =
    0b1 << 7 |     //start conversion
    modeIO << 4 |  //choose I/O
    rangeV << 1 |  //choose V range
    0b1;           //oneshot mode
  writeBuf[2] =
    SPS << 5 |     //choose SPS
    0b00011;       //we do not implement comparator or alert function
    
  if (write(ads_fd, writeBuf, 3) != 3) {
    perror("Write to register 1");
    exit (1);
  }
  usleep(500000);

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
  
  // could also multiply by 256 then add readBuf[1]
  val = readBuf[0] << 8 | readBuf[1];
  double tension = val * stepV;  
  printf("[%ld] : ADS1115 read : %d, U = %f V\n",time(NULL),val,tension);
}
