#ifndef Motion_H
#define Motion_H
#include "DYN4.h"
#include "ForceGauge.h"
#include "StateMachine.h"

#define STACK_SIZE 200

typedef struct motion_t
{
    int force;    //Read only, force value
    int position; //Read only, encoder position
    int time;     //Read only, cog sample time

    int servoPosition; //Read and write, the servo position
} Motion_Cog;
void runMotion(void *par);

#endif
