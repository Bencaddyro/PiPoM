#include <stdint.h>
#include <argp.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

#include "ads1115.h"


const char *argp_program_version = "PiPoM V1.0.0";
const char *argp_program_bug_address = "<bencaddyro@protonmail.fr>";
static char doc[] = "PiPoM, Pi Power Measurement";
static char args_doc[] = "output";
static struct argp_option options[] = {
  { "quiet",        'q',             0, 0, "Quiet mode, no debug message"},
  { "address",      'a',        "ADDR", 0, "I2C address of ADS1115, in hexadecimal format"},
  { "frequency",    'f',        "FREQ", 0, "Sample per second, accept only [8, 16, 32, 64, 128, 250, 475, 860]"},
  { "time",         't',        "TIME", 0, "Sampling duration, infinite if 0, terminate with SIGINT"},
  { "network-log",  777,  "True/False", 0, "Output log on network, default=false"},
  { "local-log",    778,  "True/False", 0, "Output log on local files, default=true"},
  { "output-log",   779,  "True/False", 0, "Output log on standard output, default=true"},
  { 0 }

};

struct arguments {
  char *args[1];
  int time;
  uint8_t frequence;
  uint8_t address;
  char verbose;
  char networkLog;
  char localLog;
  char outputLog;
  int t_frequence;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;
  switch (key) {
  case 'f':
    switch(atoi(arg)) {
    case 8 :
      arguments->frequence = SPS8;
      arguments->t_frequence = TS8;
      break;
    case 16 :
      arguments->frequence = SPS16;
      arguments->t_frequence = TS16;
      break;
    case 32 :
      arguments->frequence = SPS32;
      arguments->t_frequence = TS32;
      break;
    case 64 :
      arguments->frequence = SPS64;
      arguments->t_frequence = TS64;
      break;
    case 128 :
      arguments->frequence = SPS128;
      arguments->t_frequence = TS128;
      break;
    case 250 :
      arguments->frequence = SPS250;
      arguments->t_frequence = TS250;
      break;
    case 475 :
      arguments->frequence = SPS475;
      arguments->t_frequence = TS475;
      break;
    case 860 :
      arguments->frequence = SPS860;
      arguments->t_frequence = TS860;
      break;
    default:
      printf("Support only sampling frequence [8, 16, 32, 64, 128, 250, 475, 860]\n");
      argp_usage(state);
      break;
    };
    break;
  case 777:
    if (strcmp("True", arg)) {
      arguments->networkLog = false;
    }
    else{
      arguments->networkLog = true;
    }
    break;
  case 778:
    if (strcmp("True", arg)) {
      arguments->localLog = false;
    }
    else{
      arguments->localLog = true;
    }
    break;
  case 779:
    if (strcmp("True", arg)) {
      arguments->outputLog = false;
    }
    else{
      arguments->outputLog = true;
    }
    break;
  case 'a':
    arguments->address = (uint8_t)atoi(arg);
    break;
  case 't':
    arguments->time = atoi(arg);
    break;
  case 'q':
    arguments->verbose = false;
    break;
  case ARGP_KEY_ARG:
    if (state->arg_num >= 1)
      /* Too many arguments. */
      argp_usage (state);
    arguments->args[state->arg_num] = arg;
    break;
    
  case ARGP_KEY_END:
    if (state->arg_num < 1)
      /* Not enough arguments. */
      argp_usage (state);
    break;
    
  default: return ARGP_ERR_UNKNOWN;
  }   
  return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc, };

int main(int argc, char *argv[])
{
  signal(SIGINT, handle_sigUSR);
  signal(SIGALRM, handle_sigUSR);
  
  struct arguments arguments;

  arguments.frequence = SPS8;
  arguments.t_frequence = TS8;
  arguments.address = 0x48;
  arguments.time = 0;
  arguments.verbose = true;
  arguments.networkLog = false;
  arguments.localLog = true;
  arguments.outputLog = true;

  argp_parse(&argp, argc, argv, 0, 0, &arguments);
  //printf ("OUTPUT_FILE = %s\nTIME = %d\nFREQUENCE = %d\nADDRESS = %d\n",
  //arguments.args[0],
  //arguments.time,
  //arguments.frequence,
  //arguments.address);

  
  //-----SETUP-----

  // setup conf for power measure
  //---------------------------------------------------
  conf_ADS tension;
  
  tension.mode_IO = A1GND;  //Measure RaspBerry tension
  tension.mode = 0b1;       //Oneshot mode
  tension.status = 0b1;     //Start oneshot
  //Sample per second
  tension.SPS = arguments.frequence;
  tension.step_SPS = arguments.t_frequence;
  //Tension range
  tension.range_V = V6144;  //Mean range ~5V
  tension.step_V = S6144;

  conf_ADS current;
  current.mode_IO = A0A1;   //Measure shunt tension
  current.mode = 0b1;       //Oneshot mode
  current.status = 0b1;     //Start oneshot
  //Sample per second
  current.SPS = arguments.frequence;
  current.step_SPS = arguments.t_frequence;
  //Current range
  current.range_V = V0256;
  current.step_V = S0256;    
  //---------------------------------------------------


  // setup Log file
  FILE *log_fd = NULL;
  if(arguments.localLog)
    log_fd = fopen(arguments.args[0],"w");

  // setup Log network socket
  int net_fd = 0;
  if(arguments.networkLog){
    net_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in sin = { 0 };
    sin.sin_addr.s_addr = inet_addr("192.168.10.1");   
    sin.sin_family = AF_INET;
    sin.sin_port = htons(60030);
    connect(net_fd, (struct sockaddr*)&sin, sizeof(sin));
  }

  // setup stop signal if any
  if(arguments.time != 0){
    alarm(arguments.time);
  }

  // setup ADS1115 communication
  int ads_fd;
  setup(arguments.address, &ads_fd);

  //-----MESURE-----
  
  powerLog(tension, current, log_fd, net_fd, arguments.outputLog, arguments.verbose, ads_fd);

  //-----FIN-----

  //close file descriptor & remaining socket
  if (arguments.networkLog)
    close(net_fd);
  if (arguments.localLog)
    fclose(log_fd);

  close(ads_fd);
  
  return 0;
}
