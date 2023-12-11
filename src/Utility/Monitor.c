#include <stdbool.h>
#include <stdio.h>
#include <propeller2.h>
#include <string.h>
#include "Utility/Monitor.h"
#include "Utility/StateMachine.h"
#include "Utility/Debug.h"
#include "Utility/Motion.h"
#include "ForceGauge/ForceGauge.h"
#include "Encoder.h"
#include "Main/MaD.h"
#include "Memory/CogStatus.h"

#ifndef MOUNT_PATH
#define MOUNT_PATH "/sd"
#endif

extern long motion_position_steps;
static ForceGauge forceGauge;
static Encoder encoder;

bool loaded_mp = false;
static bool monitorLogData;
static char gcode[200] = "";
static char header[512] = "";
static char test_path[219] = MOUNT_PATH "/test.csv";

static const char profile_path[] = MOUNT_PATH "/profile.bin";
static const char test_path_bin[] = MOUNT_PATH "/test.bin";

bool monitor_set_test_name(char *name)
{
  if (strlen(name) > 8)
  {
    DEBUG_ERROR("%s","Test name too long\n");
    return false;
  }
  
  strncpy(test_path, MOUNT_PATH, strlen(MOUNT_PATH)+1);
  strncpy(test_path, "/", 5);
  strncat(test_path, name, 8);
  strncat(test_path, ".csv", 4);
  return true;
}

bool monitor_set_header(char *h)
{
  if (strlen(header) > 512)
  {
    DEBUG_ERROR("%s","Header too long\n");
    return false;
  }
  
  strncpy(header, h, 512);
  return true;
}

static bool gcode_locked = false;
void monitor_send_move(int g, int x, int f, int p)
{
  while (gcode_locked)
  {

  } 
  snprintf(gcode, 200, "G%d X%d F%d P%d", g, x, f, p);
  gcode_locked = true;
}

void monitor_run_test()
{
  monitorLogData = true;
}

static bool get_force(int lastLog)
{
  if (forceGauge.responding)
  {
    state_machine_set(PARAM_MACHINE_FORCE_GAUGE_COM, 1);
    return forceGauge.counter != lastLog;
  }
  static int last_error = 0;
  if ((_getms() - last_error) > 3000)
  {
    DEBUG_ERROR("%s","Force Gauge not responding\n");
    last_error = _getms();
  }
  // maybe force gauge should do this reconnect itself?
  state_machine_set(PARAM_MACHINE_FORCE_GAUGE_COM, 0);
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
  if (sd_card->sd_card_state != SD_CARD_IDLE && sd_card->sd_card_state != SD_CARD_SUCCESS)
  {
    mount(MOUNT_PATH, _vfs_open_sdcard());
  }
  switch(sd_card->sd_card_state)
  {
    case SD_CARD_IDLE:
      break;
    case SD_CARD_SUCCESS:
      break;
    case SD_CARD_READ_MACHINE_PROFILE:
      {
        FILE *file_config_read = fopen(profile_path, "r");
        
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
          DEBUG_ERROR("incorrect number of bytes read: %d\n", n);
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
        FILE *file = fopen(test_path_bin, "r");
        if (file == NULL)
        {
          DEBUG_ERROR("%s","Failed to open file test.bin for reading\n");
          sd_card->sd_card_state = SD_CARD_ERROR;
          break;
        }
        
        fseek(file, sd_card->read_data_index * sizeof(MonitorData), SEEK_SET);
        if (sd_card->sd_card_data == NULL)
        {
          DEBUG_ERROR("%s","sd_card_data is null\n");
          sd_card->sd_card_state = SD_CARD_ERROR;
          break;
        }
        size_t n = fread(sd_card->sd_card_data, sizeof(MonitorData), sd_card->read_data_count, file);
        fclose(file);
        sd_card->read_data_count = n/sizeof(MonitorData);
        if (sd_card->read_data_count <= 0)
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
        FILE *file = fopen(test_path_bin, "r");
        if (file == NULL)
        {
          DEBUG_ERROR("%s","Failed to open file test.bin for reading\n");
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
    case SD_CARD_WRITE_MACHINE_PROFILE:
      {
        FILE *file_config_write = fopen(profile_path, "w");
        if (file_config_write == NULL)
        {
          DEBUG_ERROR("%s","Failed to open file machine profile for writing\n");
          sd_card->sd_card_state = SD_CARD_ERROR;
          break;
        }
        DEBUG_INFO("%s","Writing Machine Profile to SD Card\n");
        int n = fwrite(&sd_card->sd_card_profile, sizeof(MachineProfile), 1, file_config_write);
        if (n != 1)
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
  if (sd_card->sd_card_state != SD_CARD_IDLE && sd_card->sd_card_state != SD_CARD_SUCCESS)
  {
    umount(MOUNT_PATH);
  }
  unlock_and_monitor_sd_card();
}

bool get_monitor_data(MonitorData *data, int timeout_ms)
{
  MonitorData *monitor_data = lock_monitor_data_ms(timeout_ms);
  if (monitor_data == NULL)
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

static bool trigger_set_gauge_force = false;
void set_gauge_force()
{
  trigger_set_gauge_force = true;
}

static void load_machine_profile()
{
    DEBUG_INFO("%s","Loading machine profile\n");
    mount(MOUNT_PATH, _vfs_open_sdcard());
    FILE *mp = fopen(profile_path, "r");
    if (mp == NULL)
    {
       // Load default profile, one does not exist
        DEBUG_WARNING("%s","No machine profile found, loading default\n");
        MachineProfile temp_profile;
        memset(&temp_profile, 0, sizeof(MachineProfile));
        strcpy(temp_profile.name, "DEFAULT");
        FILE* mp_temp = fopen(profile_path, "w");
        if (mp_temp == NULL)
        {
            DEBUG_ERROR("%s","Failed to open file machine profile for writing\n");
            _waitms(1000);
            _reboot();
        }
        int n = fwrite(&temp_profile, sizeof(MachineProfile), 1, mp_temp);
        if (n != 1)
        {
            DEBUG_ERROR("incorrect number of bytes written: %d\n", n);
            _waitms(1000);
            _reboot();
        }
        fclose(mp_temp);
    }
    
    DEBUG_INFO("%s","Reading sd profile from sd\n");
    MachineProfile machine_profile_temp;
    size_t n = fread(&machine_profile_temp, sizeof(MachineProfile), 1, mp);
    if (n != 1)
    {
        DEBUG_ERROR("incorrect number of bytes read: %d\n", n);
        _waitms(1000);
        _reboot();
    }

    DEBUG_INFO("%s","Loading machine profile from SD card\n");
    
    // Load profile from SD card
    MachineProfile *machine_profile;
    if (!lock_machine_profile_ms(&machine_profile, 100))
    {
        DEBUG_ERROR("%s","Failed to lock machine profile, default should have been loaded!\n");
        _waitms(1000);
        _reboot();
    }
    memcpy(machine_profile, &machine_profile_temp, sizeof(MachineProfile));
    unlock_machine_profile();

    DEBUG_NOTIFY("%s","Machine profile loaded from SD Card\n");
    set_machine_profile_loaded(true);
    umount(MOUNT_PATH);
    return;
    
}

/*responsible for reading/writing data to buffer/test output*/
static void monitor_cog(int samplerate)
{
  if (init_monitor_state() == -1)
  {
    DEBUG_ERROR("%s","Failed to initialize monitor state REBOOTING\n");
    _reboot();
  }

  load_machine_profile();

  // Save copy of machine profile
  MachineProfile machine_profile;
  MachineProfile *profile_ptr = NULL;
  if (!lock_machine_profile_ms(&profile_ptr,1000))
  {
    DEBUG_ERROR("%s","Failed to lock machine profile REBOOTING\n");
    _waitms(1000);
    _reboot();
    return;
  }
  else
  {
    if (profile_ptr == NULL)
    {
      DEBUG_ERROR("%s","Machine profile is NULL REBOOTING\n");
       _waitms(1000);
      _reboot();
      return;
    }
    else
    {
      memcpy(&machine_profile, profile_ptr, sizeof(MachineProfile));
    }
  }
  unlock_machine_profile();
  
  DEBUG_INFO("%s","Starting Monitor Cog\n");

  FILE *testFileCsv = NULL;
  FILE *testFileBin = NULL;

  monitorLogData = false;

  // Set up force gauge
  force_gauge_begin(&forceGauge, FORCE_GAUGE_RX, FORCE_GAUGE_TX);
  
  // Set up encoder
  encoder_start(&encoder, SERVO_ENCODER_A, SERVO_ENCODER_B, -1, false, 0, -100000, 100000);
  long lastTime = _getms();
  int force_count = 0;
  int force_raw = 0;
  int gauge_length = 0;
  int gauge_force = 0;
  int test_start_time = 0;
  MonitorData monitor_data_local;
  while (1)
  {
    set_monitor_status(_getms());
    long start = _getus();
    bool update = false;

    if (get_force(force_count))
    {
      if (force_count - forceGauge.counter > 1)
      {
        DEBUG_WARNING("Force gauge data dropped: %d\n", force_count - forceGauge.counter);
      }
      force_count = forceGauge.counter; // Increment when new data is added to buffer, used for checking if data is new.
      force_raw = forceGauge.forceRaw;
      update = true;
    }
    else
    {
      force_raw = 0; // Force gauge not responding, set to 0
    }
    
    long forceus = _getus();

    if (update)
    {
      set_monitor_status(_getms());
        monitor_data_local.log = force_count; // Increment when new data is added to buffer, used for checking if data is new.
        monitor_data_local.forceRaw = force_raw;
        monitor_data_local.encoderRaw = encoder_value(&encoder);
        monitor_data_local.timems = _getms();
        monitor_data_local.timeus = _getus();
        monitor_data_local.forcemN = raw_to_force(monitor_data_local.forceRaw, machine_profile.configuration.forceGaugeOffset, machine_profile.configuration.forceGaugeGain) - gauge_force;
        monitor_data_local.encoderum = monitor_data_local.encoderRaw*1000/machine_profile.configuration.encoderStepsPermm;
        monitor_data_local.gauge = monitor_data_local.encoderum - gauge_length;
        monitor_data_local.force = monitor_data_local.forcemN / 1000.0; // Convert Force to N
        monitor_data_local.position = monitor_data_local.encoderRaw/machine_profile.configuration.encoderStepsPermm;
        monitor_data_local.setpoint = motion_get_setpoint()*1000/machine_profile.configuration.encoderStepsPermm;
        if (trigger_set_gauge)
        {
          DEBUG_NOTIFY("%s","Gauge Length Set!");
          trigger_set_gauge = false;
          encoder_set(&encoder, 0);
          motion_set_position(0);
        }
        if (trigger_set_gauge_force)
        {
          DEBUG_NOTIFY("%s","Gauge Force Set!");
          trigger_set_gauge_force = false;
          gauge_force = raw_to_force(monitor_data_local.forceRaw, machine_profile.configuration.forceGaugeOffset, machine_profile.configuration.forceGaugeGain);
        }
    }

    long update_us = _getus();
    
    if (monitorLogData)
    {
      if (testFileCsv == NULL)
      {
        // Begin Logging data
        mount(MOUNT_PATH, _vfs_open_sdcard());
        testFileCsv = fopen(test_path, "w");
        testFileBin = fopen(test_path_bin, "w");
        if (testFileCsv == NULL || testFileBin == NULL)
        {
          DEBUG_ERROR("Failed to open file %s for writing\n", test_path);
          umount(MOUNT_PATH);
          _waitms(100);
          mount(MOUNT_PATH, _vfs_open_sdcard());
        }
        fprintf(testFileCsv, "%s\n", header);
        fprintf(testFileCsv, "Time,Force,Distance,Setpoint\n");
        if (state_machine_set(PARAM_MOTION_MODE, MODE_TEST_RUNNING))
        {
           DEBUG_NOTIFY("%s","Starting Test!\n");
           test_start_time = _getus();
        }
        else
        {
          monitorLogData = false;
        }
      }

      MachineState machineState;
      get_machine_state(&machineState);
      if (monitorLogData && (machineState.motionParameters.mode != MODE_TEST_RUNNING))
      {
        // Test is over!
        monitorLogData = false;
      }
      
      if (monitorLogData && update && testFileCsv != NULL)
      {
        // Write data to file
        while (gcode_locked) // wait for motion to finish writing gcode
        {
          if (strncmp(gcode,"",1) != 0) // if gcode is not empty
          {
            fprintf(testFileCsv, "%s\n", gcode); // embedd gcode in csv file
            strncpy(gcode,"",1); // clear gcode
            gcode_locked = false; // allow motion to write new gcode
          }
        }
        monitor_data_local.timeus = _getus() - test_start_time;
        fwrite(&monitor_data_local, sizeof(MonitorData), 1, testFileBin);
        fprintf(testFileCsv, "%d,%d,%d,%d\n", monitor_data_local.timeus, monitor_data_local.forcemN, monitor_data_local.encoderum, monitor_data_local.setpoint);
      }
    }
    else if (testFileCsv != NULL)
    {
      fclose(testFileCsv);
      fclose (testFileBin);
      umount(MOUNT_PATH);
      testFileCsv = NULL;
      testFileBin = NULL;
    }
    else
    {
      // Execute SD card commands
      read_sd();
    }

    MonitorData * monitor_data_global = lock_monitor_data_us(100);
    if (monitor_data_global != NULL)
    {
      memcpy(monitor_data_global, &monitor_data_local, sizeof(MonitorData));
      unlock_monitor_data();
    }
    else
    {
      DEBUG_WARNING("%s","Failed to lock monitor data while writing to sd card\n");
    }

    long write_sd_us = _getus();
    if (update && testFileCsv != NULL)
    {
      //DEBUG_INFO("ForceUS: %lu, UpdateUS: %lu, WriteSDUS: %lu, Total: %lu\n", forceus-start, update_us-forceus, write_sd_us-update_us, write_sd_us-start);
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
  if (!lock_sd_card_ms(&sd_card,100))
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
  return sd_card_file_exists(profile_path);
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

  MonitorSDCard *sd_card = NULL;
  if (!lock_sd_card_ms(&sd_card,1000))
  {
    DEBUG_WARNING("Failed to lock monitor for reading data: running=%d\n", sd_card->sd_card_state);
    return false;
  }

  sd_card->read_data_count = count;
  sd_card->read_data_index = index;
  sd_card->sd_card_data = (char *)data;
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
  sd_card->sd_card_data = NULL;
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
  if (!lock_sd_card_ms(&sd_card,1000))
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
      DEBUG_ERROR("%s","Timeout waiting for SD card to read data size\n");
      break;
    }
  }

  if (!lock_sd_card_ms(&sd_card,10))
  {
    DEBUG_WARNING("%s","Failed to lock monitor for reading data size\n");
    return false;
  }

  int data_count = sd_card->read_data_count;
  if (*state != SD_CARD_SUCCESS)
  {
    DEBUG_WARNING("%s","Failed to read data size\n");
    data_count = 0;
  }

  unlock_sd_card();
  DEBUG_INFO("Read data size: %d\n", data_count);
  return data_count;
}

// Uses STDLIB, does not begin new cog and this function will never exit
void monitor_begin(int sampleRate)
{
  monitor_cog(sampleRate);
  return;
}
