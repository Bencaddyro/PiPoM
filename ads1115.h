


typedef struct{
  uint8_t SPS;
  uint8_t mode;
  uint8_t range_V;
  uint8_t mode_IO;
  uint8_t status;
  double step_V;
  int step_SPS;
} conf_ADS;



void handle_sigUSR(int sig);
void setup(int ads_addr, int *addr_fd);

void powerLog(conf_ADS tension, conf_ADS current, FILE *log_fd, int net_fd, char output, char verbose, int ads_fd);


//void oneshot(int ads_fd, uint8_t modeIO, uint8_t rangeV, double stepV, uint8_t SPS);
//void continuous(int ads_fd, uint8_t modeIO, uint8_t rangeV, double stepV, uint8_t SPS, int t_SPS, FILE *log_fd, int net_fd, char verbose);


//Mode for I/O
#define A0A1     0b000
#define A0A3     0b001
#define A1A3     0b010
#define A2A3     0b011
#define A0GND    0b100
#define A1GND    0b101
#define A2GND    0b110
#define A3GND    0b111
//Mode for V range
#define V6144    0b000
#define V4096    0b001
#define V2048    0b010
#define V1024    0b011
#define V0512    0b100
#define V0256    0b101
//Step for V range
#define S6144    .0001875
#define S4096    .000125
#define S2048    .0000625
#define S1024    .00003125
#define S0512    .000015625
#define S0256    .0000078125
//Mode for SPS
#define SPS8     0b000
#define SPS16    0b001
#define SPS32    0b010
#define SPS64    0b011
#define SPS128   0b100
#define SPS250   0b101
#define SPS475   0b110
#define SPS860   0b111
//Time for SPS
#define TS8     125000
#define TS16     62500
#define TS32     31250
#define TS64     15625
#define TS128     7812
#define TS250     4000
#define TS475     2105
#define TS860     1163
