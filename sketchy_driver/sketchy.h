#ifndef SKETCHY_H
#define SKETCHY_H

#include "Point.h"

int run(void (*executeMotion)());
void sketchy_suspend();
void sketchy_resume();
int autoNull();

#endif
