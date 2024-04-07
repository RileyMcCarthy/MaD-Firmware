#ifndef MONITOR_STATE_H
#define MONITOR_STATE_H
#include "MachineProfile.h"
#include <inttypes.h>

typedef enum SDCardState
{
  SD_CARD_SUCCESS,
  SD_CARD_IDLE, // waiting for next command, write or read is comleted!
  SD_CARD_READ_MACHINE_PROFILE, // read machine profile from SD card, updated global struct (locked)
  SD_CARD_READ_DATA,
  SD_CARD_READ_DATA_SIZE,
  SD_CARD_WRITE_MACHINE_PROFILE,
  SD_CARD_FILE_EXISTS,
  SD_CARD_ERROR
} SDCardState;

typedef struct MonitorData
{
    int forceRaw;        // Raw force value
    int encoderRaw;      // Raw encoder value
    int forcemN;         // Force in mN
    int encoderum;       // Encoder in um
    int gauge;           // Gauge value in mN
    double force;        // Force value in N
    double position;     // Position value in mm
    int setpoint;        // Setpoint in um
    unsigned int timems; // time in ms
    unsigned int timeus; // time in us
    int log;
} MonitorData;

typedef struct MonitorSDCard
{
  SDCardState sd_card_state;
  MachineProfile sd_card_profile; // Storage for machine profile read from SD card
  char *sd_card_data; // Storage for sd card data read from SD card
  char sd_card_file_name[255]; // Storage for file name read from SD card
  int read_data_index;
  int read_data_count;

  bool is_writing;
} MonitorSDCard;

int init_monitor_data();
int init_monitor_state();
MonitorData * lock_monitor_data();
MonitorData * lock_monitor_data_ms(int ms);
MonitorData * lock_monitor_data_us(int us);
bool unlock_monitor_data();
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
bool lock_sd_card(MonitorSDCard ** data);
bool lock_sd_card_ms(MonitorSDCard ** data, uint32_t ms);
bool unlock_sd_card();
SDCardState * unlock_and_monitor_sd_card();
bool lock_sd_card();
#endif // MONITOR_STATE_H
