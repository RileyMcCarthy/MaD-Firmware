#ifndef _COGSTATUS_H_
#define _COGSTATUS_H_

#include <stdint.h>
#include <stdbool.h>

void set_motion_status(uint32_t statusms);
void set_control_status(uint32_t statusms);
void set_communication_status(uint32_t statusms);
void set_monitor_status(uint32_t statusms);
bool get_machine_status();

#endif // _COGSTATUS_H_
