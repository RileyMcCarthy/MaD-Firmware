#ifndef JSONENCODER_H
#define JSONENCODER_H
/** Encoding API **/
#include "StateMachine.h"
#include "dev_nvram.h"
#include "app_monitor.h"
#include "app_monitor.h"
#include "app_notification.h"

char *notification_to_json(app_notification_message_S *notification);
char *machine_state_to_json(MachineState *state);
char *machine_profile_to_json(MachineProfile *settings);
char *sample_profile_to_json(SampleProfile *sample);
char *test_data_to_json(app_monitor_sample_t *data, int count, int index);
#endif // JSONENCODER_H
