#ifndef JSONENCODER_H
/** Encoding API **/
#include "StateMachine.h"
#include "Memory/MachineProfile.h"
#include "Monitor.h"
#include "Communication/Communication.h"

char * notification_to_json(Notification * notification);
char *machine_state_to_json(MachineState *state);
char *machine_profile_to_json(MachineProfile *settings);
char *sample_profile_to_json(SampleProfile *sample);
char *test_data_to_json(MonitorData *data, int count, int index);
bool unlock_json_buffer();
bool lock_json_buffer();
#endif // JSONENCODER_H
