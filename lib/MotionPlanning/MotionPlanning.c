#include "MotionPlanning.h"

double steps_to_mm(int steps, double diameter, double stepmm)
{
    return steps * (diameter * 3.14159) / stepmm;
}

int steps_to_um(int steps, double diameter, double stepmm)
{
    return (int)round(steps * (diameter * 3.14159) / stepmm * 1000);
}

int mm_to_steps(double mm, double diameter, double stepmm)
{
    return (int)round(mm * stepmm / (double)(diameter * 3.14159));
}
