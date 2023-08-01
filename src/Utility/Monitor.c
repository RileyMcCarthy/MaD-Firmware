#include "Utility/Monitor.h"
#include "Utility/StateMachine.h"
#include "Utility/Debug.h"
#include "ForceGauge.h"
#include "Encoder.h"
#include <stdbool.h>
#include <stdio.h>
#include <propeller.h>
#include "Main/MaD.h"

extern long motion_position_steps;
static ForceGauge forceGauge;
static Encoder encoder;

bool loaded_mp = false;
static bool monitorLogData;

static bool get_force(int lastLog)
{
  if (forceGauge.responding)
  {
    return forceGauge.counter != lastLog;
  }
  DEBUG_ERROR("%s","Force Gauge disconnected, attempting to reconnect\n");
  // maybe force gauge should do this reconnect itself?
  force_gauge_stop(&forceGauge);
  if (force_gauge_begin(&forceGauge, FORCE_GAUGE_RX, FORCE_GAUGE_TX))
  {
    //printf("Force Gauge reconnected\n");
    DEBUG_INFO("%s","Force Gauge reconnected\n");
    state_machine_set(PARAM_MACHINE_FORCE_GAUGE_COM, 1);
  }
  else
  {
    DEBUG_ERROR("%s","Force Gauge failed to reconnect\n");
    state_machine_set(PARAM_MACHINE_FORCE_GAUGE_COM, 0);
  }
  return false;
}

// Lock monitor state and modify sd_card_state
// unlock monitor state
static void read_sd()
{
  MonitorSDCard *sd_card = NULL;
  if (!lock_sd_card_ms(&sd_card,10))
  {
    DEBUG_ERROR("%s","Failed to lock monitor sd card, something is taking too long...\n");
    return;
  }
  switch(sd_card->sd_card_state)
  {
    case SD_CARD_IDLE:
      break;
    case SD_CARD_READ_MACHINE_PROFILE:
      {
        FILE *file_config_read = fopen("/sd/profile.bin", "r");
        
        if (file_config_read == NULL)
        {
          DEBUG_ERROR("%s","Failed to open machine profile for reading\n");
          sd_card->sd_card_state = SD_CARD_ERROR;
          break;
        }

        int n = fread(&sd_card->sd_card_profile, sizeof(MachineProfile), 1, file_config_read);
        fclose(file_config_read);

        if (n != sizeof(MachineProfile))
        {
          DEBUG_ERROR("%s","incorrect number of bytes read: %d\n", n);
          sd_card->sd_card_state = SD_CARD_ERROR;
          break;
        }
        
        DEBUG_INFO("read_profile_status: %s\n", sd_card->sd_card_profile.name);
        DEBUG_INFO("offset %d\n", sd_card->sd_card_profile.configuration.forceGaugeOffset);
        sd_card->sd_card_state = SD_CARD_SUCCESS;
        break;
      }
    case SD_CARD_READ_DATA:
      {
        FILE *file = fopen("/sd/test.txt", "r");
        if (file == NULL)
        {
          DEBUG_ERROR("%s","Failed to open file test.txt for reading\n");
          sd_card->sd_card_state = SD_CARD_ERROR;
          break;
        }
        
        fseek(file, sd_card->read_data_index * sizeof(MonitorData), SEEK_SET);
        int n = fread(sd_card->sd_card_data, sizeof(MonitorData), sd_card->read_data_count, file);
        fclose(file);
        sd_card->read_data_count = n/sizeof(MonitorData);
        if (sd_card->read_data_count == 0)
        {
          DEBUG_ERROR("%s","incorrect number of bytes read: %d\n", n);
          sd_card->sd_card_state = SD_CARD_ERROR;
          break;
        }
        sd_card->sd_card_state = SD_CARD_SUCCESS;
        break;
      }
    case SD_CARD_READ_DATA_SIZE:
      {
        FILE *file = fopen("/sd/test.txt", "r");
        if (file == NULL)
        {
          DEBUG_ERROR("%s","Failed to open file test.txt for reading\n");
          sd_card->sd_card_state = SD_CARD_ERROR;
          break;
        }
        
        fseek(file, 0, SEEK_END);
        int n = ftell(file);
        fclose(file);
        sd_card->read_data_count = n/sizeof(MonitorData);
        if (sd_card->read_data_count == 0)
        {
          DEBUG_ERROR("%s","incorrect number of bytes read: %d\n", n);
          sd_card->sd_card_state = SD_CARD_ERROR;
          break;
        }
        sd_card->sd_card_state = SD_CARD_SUCCESS;
        break;
      }
    case SD_CARD_WRITE_DATA:
      {
        FILE *file = fopen("/sd/test.txt", "a");
        if (file == NULL)
        {
          DEBUG_ERROR("%s","Failed to open file test.txt for writing\n");
          sd_card->sd_card_state = SD_CARD_ERROR;
          break;
        }
        
        int n = fwrite(sd_card->sd_card_data, sizeof(MonitorData), sd_card->read_data_count, file);
        fclose(file);
        sd_card->read_data_count = n/sizeof(MonitorData);
        if (sd_card->read_data_count == 0)
        {
          DEBUG_ERROR("%s","incorrect number of bytes written: %d\n", n);
          sd_card->sd_card_state = SD_CARD_ERROR;
          break;
        }
        sd_card->sd_card_state = SD_CARD_SUCCESS;
        break;
      }
    case SD_CARD_WRITE_MACHINE_PROFILE:
      {
        FILE *file_config_write = fopen("/sd/profile.bin", "w");
        if (file_config_write == NULL)
        {
          DEBUG_ERROR("%s","Failed to open file machine profile for writing\n");
          sd_card->sd_card_state = SD_CARD_ERROR;
          break;
        }
        DEBUG_INFO("%s","Writing Machine Profile to SD Card\n");
        int n = fwrite(&sd_card->sd_card_profile, sizeof(MachineProfile), 1, file_config_write);
        if (n != sizeof(MachineProfile))
        {
          DEBUG_ERROR("incorrect number of bytes written: %d\n", n);
          sd_card->sd_card_state = SD_CARD_ERROR;
          break;
        }
        fclose(file_config_write);
        if (n != sizeof(MachineProfile))
        {
          DEBUG_ERROR("incorrect number of bytes written: %d\n", n);
          sd_card->sd_card_state = SD_CARD_ERROR;
          break;
        }
        sd_card->sd_card_state = SD_CARD_SUCCESS;
        loaded_mp = false;
        break;
      }
    case SD_CARD_FILE_EXISTS:
    {
      FILE *file = fopen(sd_card->sd_card_file_name, "r");
      if (file == NULL)
      {
        DEBUG_ERROR("Failed to open file %s for reading\n", sd_card->sd_card_file_name);
        sd_card->sd_card_state = SD_CARD_ERROR;
        break;
      }
      fclose(file);
      sd_card->sd_card_state = SD_CARD_SUCCESS;
      break;
    }
  }
  unlock_and_monitor_sd_card();
}

bool get_monitor_data(MonitorData *data, int timeout_ms)
{
  MonitorData *monitor_data;
  if (!lock_monitor_data_ms(&monitor_data, timeout_ms))
  {
    DEBUG_WARNING("%s","Failed to lock monitor data\n");
    return false;
  }

  memcpy(data, monitor_data, sizeof(MonitorData));

  unlock_monitor_data();
  return true;
}

static bool trigger_set_gauge = false;
void set_gauge_length()
{
trigger_set_gauge = true;
}

/*responsible for reading/writing data to buffer/test output*/
static void monitor_cog(int samplerate)
{
  init_monitor_state();

  FILE *testFile = NULL;

  MachineProfile machine_profile;
  monitorLogData = false;
  
  // Set up encoder
  encoder.start(SERVO_ENCODER_A, SERVO_ENCODER_B, -1, false, 0, -100000, 100000);
  long lastTime = _getms();
  int force_count = 0;
  int force_raw = 0;
  int gauge_length = 0;
  while (1)
  {
    long start = _getus();
    bool update = false;
    if (machine_profile_loaded() && !loaded_mp)
    {
      MachineProfile *profile_ptr = NULL;
      while (!lock_machine_profile_ms(&profile_ptr,1000))
      {
        DEBUG_ERROR("%s","Failed to lock machine profile\n");
      }
      memcpy(&machine_profile, profile_ptr, sizeof(MachineProfile));
      unlock_machine_profile();
      loaded_mp = true;
    }

    if (get_force(force_count))
    {
      force_count = forceGauge.counter; // Increment when new data is added to buffer, used for checking if data is new.
      force_raw = forceGauge.forceRaw;
      update = true;
    }
    else
    {
      force_raw = 0; // Force gauge not responding, set to 0
    }
    
    long forceus = _getus() - start;

    MonitorData *monitor_data;
    if (update)
    if (lock_monitor_data_ms(&monitor_data, 1000))
    {
      monitor_data->log = monitor_data->log + 1; // Increment when new data is added to buffer, used for checking if data is new.
      monitor_data->forceRaw = force_raw;
      monitor_data->encoderRaw = encoder.value();
      monitor_data->timems = _getms();
      monitor_data->timeus = _getus();
      monitor_data->forcemN = raw_to_force(monitor_data->forceRaw, machine_profile.configuration.forceGaugeOffset, machine_profile.configuration.forceGaugeGain);
      monitor_data->encoderum = motion_get_position()*1000/machine_profile.configuration.encoderStepsPermm;
      monitor_data->gauge = monitor_data->encoderum - gauge_length;
      monitor_data->force = monitor_data->forcemN / 1000.0; // Convert Force to N
      monitor_data->position = motion_get_position()*1000/machine_profile.configuration.encoderStepsPermm;
      monitor_data->setpoint = motion_get_setpoint()*1000/machine_profile.configuration.encoderStepsPermm;
      if (trigger_set_gauge)
      {
        trigger_set_gauge = false;
        gauge_length = monitor_data->encoderum;
      }
      unlock_monitor_data();
      update = true;
    }
    else
    {
      DEBUG_WARNING("%s","Failed to lock monitor data\n");
    }

    long encoderus = _getus() - start - forceus;
    
    MachineState machineState;
    get_machine_state(&machineState);
    
    if (machineState.motionParameters.mode == MODE_TEST_RUNNING)
    {
      monitorLogData = true;
      if (testFile == NULL)
      {
        // Begin Logging data
        testFile = fopen("/sd/test.txt", "w");
        if (testFile == NULL)
        {
          DEBUG_ERROR("%s","Failed to open file test.txt for writing\n");
          state_machine_set(PARAM_MOTION_MODE, MODE_TEST);
        }else{
          fprintf(testFile,"Time (us), Force (mN), Encoder (um), Gauge (um) Setpoint (um)\n");
        }
      }
      if (update && testFile != NULL)
      {
        // Write data to file
        fprintf(testFile, "%d, %d, %d,%d\n",  monitor_data->timeus, monitor_data->forcemN, monitor_data->encoderum,monitor_data->gauge,monitor_data->setpoint);
      }
    }
    else if (testFile != NULL)
    {
      monitorLogData = false;
      // Stop Logging
      fclose(testFile);
      testFile = NULL;
    }
    else
    {
      // Execute SD card commands
      read_sd();
    }
  }
}

static bool sd_card_file_exists(const char *filename)
{
  if (monitorLogData)
  {
    return false; // Don't read from SD card if logging
  }
  
  MonitorSDCard *sd_card;
  if (!lock_sd_card(&sd_card))
  {
    DEBUG_WARNING("Failed to lock monitor checking file exists: %s\n", filename);
    return false;
  }

  if (sd_card->sd_card_state != SD_CARD_IDLE)
  {
    DEBUG_WARNING("SD card not idle when checking file exists: running=%d\n", sd_card->sd_card_state);
    unlock_and_monitor_sd_card();
    return false;
  }

  strncpy(sd_card->sd_card_file_name, filename, 255);
  sd_card->sd_card_state = SD_CARD_FILE_EXISTS;
  DEBUG_INFO("%s","Changing state to sd card file exists\n");

  SDCardState * state = unlock_and_monitor_sd_card();

  long startms = _getms();
  while (*state == SD_CARD_FILE_EXISTS) // Wait for cog to finish reading
  {
    if (_getms() - startms > 5000)
    {
      DEBUG_ERROR("Timeout waiting for SD card to check file exists: %s\n", filename);
      break;
    }
  }
  
  bool result = *state == SD_CARD_SUCCESS;
  unlock_sd_card();
  return result;
}

bool read_sd_profile(MachineProfile * profile)
{
  long startms = _getms();
  if (monitorLogData)
  {
    DEBUG_WARNING("%s","Failed to read machine profile: logging\n");
    return false; // Don't read from SD card if logging
  }
  
  MonitorSDCard *sd_card;
  if (!lock_sd_card_ms(&sd_card,10))
  {
    DEBUG_WARNING("%s","Failed to lock monitor reading machine profile\n");
    return false;
  }

  if (sd_card->sd_card_state != SD_CARD_IDLE)
  {
    DEBUG_WARNING("SD card not idle when reading machine profile: running=%d\n", sd_card->sd_card_state);
    unlock_and_monitor_sd_card();
    return false;
  }

  sd_card->sd_card_state = SD_CARD_READ_MACHINE_PROFILE;
  DEBUG_INFO("%s","Changing state to sd card read machine profile\n");

  SDCardState * state = unlock_and_monitor_sd_card();

  while (*state == SD_CARD_READ_MACHINE_PROFILE) // Wait for cog to finish reading
  {
    if (_getms() - startms > 5000)
    {
      DEBUG_ERROR("%s","Timeout waiting for SD card to read machine profile\n");
      break;
    }
  }

  if (!lock_sd_card_ms(&sd_card,10))
  {
    DEBUG_WARNING("%s","Failed to lock monitor reading machine profile\n");
    return false;
  }
  
  memcpy(profile, &(sd_card->sd_card_profile), sizeof(MachineProfile));
  bool result = *state == SD_CARD_SUCCESS;
  unlock_sd_card();
  
  return result;
}

bool sd_profile_exists()
{
  return sd_card_file_exists("/sd/profile.bin");
}

bool write_sd_profile(MachineProfile * profile)
{
  long startms = _getms();
  if (monitorLogData)
  {
    return false; // Don't write to SD card if logging
  }

  MonitorSDCard *sd_card;
  if (!lock_sd_card(&sd_card))
  {
    DEBUG_WARNING("Failed to lock monitor for writing machine profile: running=%d\n", sd_card->sd_card_state);
    return false;
  }

  memcpy(&(sd_card->sd_card_profile), profile, sizeof(MachineProfile));
  sd_card->sd_card_state = SD_CARD_WRITE_MACHINE_PROFILE;
  DEBUG_INFO("%s","Changing state to sd card write machine profile\n");

  SDCardState * sd_card_state = unlock_and_monitor_sd_card();

  while(*sd_card_state == SD_CARD_WRITE_MACHINE_PROFILE) // Wait for cog to finish writing
  {
    if (_getms() - startms > 5000)
    {
      DEBUG_ERROR("%s", "Failed to write machine profile, timeout!\n");
      break;
    }
  }
  
  bool result = *sd_card_state == SD_CARD_SUCCESS;
  unlock_sd_card();
  return result;
}

int read_sd_card_data(MonitorData *data, int index, int count)
{
  long startms = _getms();
  if (monitorLogData)
  {
    //printf("unable to read data while logging\n");
    return 0; // Don't read from SD card if logging
  }

  MonitorSDCard *sd_card;
  if (!lock_sd_card(&sd_card))
  {
    DEBUG_WARNING("Failed to lock monitor for reading data: running=%d\n", sd_card->sd_card_state);
    return false;
  }

  sd_card->read_data_count = count;
  sd_card->read_data_index = index;
  sd_card->sd_card_state = SD_CARD_READ_DATA;
  DEBUG_INFO("%s","Changing state to sd card read data\n");

  SDCardState * state = unlock_and_monitor_sd_card();

  while (*state == SD_CARD_READ_DATA) // Wait for cog to finish reading
  {
    if (_getms() - startms > 1000)
    {
      break;
    }
  }

  if (!lock_sd_card_ms(&sd_card,10))
  {
    DEBUG_WARNING("%s","Failed to lock monitor for reading data\n");
    return false;
  }

  bool result = *state == SD_CARD_SUCCESS;
  int data_count = sd_card->read_data_count;

  memcpy(data, sd_card->sd_card_data, sizeof(MonitorData)*sd_card->read_data_count);

  unlock_sd_card();
  return data_count;
}

int read_data_size()
{
  long startms = _getms();
  if (monitorLogData)
  {
    return 0; // Don't read from SD card if logging
  }
  
  MonitorSDCard *sd_card;
  if (!lock_sd_card(&sd_card))
  {
    DEBUG_WARNING("%s","Failed to lock monitor for reading data size\n");
    return false;
  }

  sd_card->sd_card_state = SD_CARD_READ_DATA_SIZE;
  DEBUG_INFO("%s","Changing state to sd card read data size\n");

  SDCardState * state = unlock_and_monitor_sd_card();

  while (*state == SD_CARD_READ_DATA_SIZE) // Wait for cog to finish reading
  {
    if (_getms() - startms > 1000)
    {
      break;
    }
  }

  if (!lock_sd_card_ms(&sd_card,10))
  {
    DEBUG_WARNING("%s","Failed to lock monitor for reading data size\n");
    return false;
  }

  int data_count = sd_card->read_data_count;
  if (*state == SD_CARD_SUCCESS)
  {
    data_count = 0;
  }

  unlock_sd_card();
  return data_count;
}

// Uses STDLIB, does not begin new cog and this function will never exit
void monitor_begin(int sampleRate)
{
  monitor_cog(sampleRate);
  return;
}
