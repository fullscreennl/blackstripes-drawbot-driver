#ifndef MACHINE_SETTINGS_H
#define MACHINE_SETTINGS_H

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

//don't set this lower than 1 wiggles will happen on axis that are stationary
#define LINE_SEGMENT_SIZE_MM 1.0
#define DEG 57.2957795

#define MAXDELAY 900000
#define MINDELAY  50000 

#define MARKER_NIB_SIZE_MM  3.2 //4.0
#define MARKER_NIB_SIZE  MARKER_NIB_SIZE_MM

#define UPPER_ARM_LENGTH 1100.0
#define LOWER_ARM_LENGTH 1300.0
#define SHOULDER_DIST 300.0
#define LEFT_SHOULDER_OFFSET -150.0
#define RIGHT_SHOULDER_OFFSET 150.0
#define SHOULDER_HEIGHT 100.0
#define LEFT_SHOULDER_POS_Y 100.0
#define RIGHT_SHOULDER_POS_Y 100.0
#define EXENSION1 98.0
#define CANVAS_Y 330
#define MOVEMENT_STEP ((4.71*13.0)/(6400.0)) //6400 microsteps, 13 tooth sprocket, rack pitch 4.71mm
#define ANGLE_PER_STEP (360.0/(3200.0*40.0)) //12800 microsteps, 1:40 gearbox
#define MAX_CANVAS_SIZE_X 10000.0
#define MAX_CANVAS_SIZE_Y 2000.0
#define BOT_REPOSITION_THRESHOLD 500.0 // can not be higher (it will destroy the limit switches)
#define NULL_DEGREES_LEFT 94.2
#define NULL_DEGREES_RIGHT 94.0

#endif
