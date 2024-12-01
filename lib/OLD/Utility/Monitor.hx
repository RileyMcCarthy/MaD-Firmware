#ifndef MONITOR_H
#define MONITOR_H
#include <stdbool.h>
#include <stdint.h>
#include "dev_nvram.h"
#include "Memory/MachineProfile.h"
#include "Memory/MonitorState.h"
#include "ForceGauge/ForceGauge.h"
#include "Encoder.h"

void monitor_begin(int sampleRate);

void set_gauge_length();
void set_gauge_force();

void monitor_send_move(int g, int x, int f, int p);
void monitor_run_test();
bool monitor_set_header(char *h);
bool monitor_set_test_name(const char *name);

// API for reading and writing machine profiles to SD card
bool read_sd_profile(MachineProfile * profile);
int read_sd_card_data(MonitorData *data, int index, int count);
int read_data_size();
bool write_sd_profile(MachineProfile * profile);
bool get_monitor_data(MonitorData *data, int timeout_ms);
void set_gauge_length();
#endif
