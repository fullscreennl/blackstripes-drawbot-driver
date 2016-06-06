#ifndef MACHINE_SETTINGS_H
#define MACHINE_SETTINGS_H

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define LINE_SEGMENT_SIZE_MM 1.0
#define DEG 57.2957795

#define MAXDELAY 900000
#define MINDELAY  50000 

#define MARKER_NIB_SIZE_MM  3.2 //4.0
#define MARKER_NIB_SIZE  MARKER_NIB_SIZE_MM

//#define machine_size_medium 1  //small/medium/large

#ifdef __VPLOTTER__

#define MAX_CANVAS_SIZE_X 1500.0
#define MAX_CANVAS_SIZE_Y 1550.0
#define CANVAS_Y 875.0
#define LEFT_STEPPER_X 0.0
#define LEFT_STEPPER_Y 0.0
#define RIGHT_STEPPER_X 2915.0
#define RIGHT_STEPPER_Y 0.0
#define STEPS_PER_MM 42.5532
#define HOME_LEFT_MM 1580.0
#define HOME_RIGHT_MM 1580.0

#else

#ifdef machine_size_medium

#define CENTER 180.0 
#define UPPER_ARM_LENGTH 252.0 
#define LOWER_ARM_LENGTH 324.0 
#define SHOULDER_DIST 108.0 
#define SHOULDER_HEIGHT 36.0 
#define LEFT_SHOULDER_POS_X (CENTER - 50.0)
#define RIGHT_SHOULDER_POS_X (CENTER + 50.0)
#define LEFT_SHOULDER_POS_Y 36.0
#define RIGHT_SHOULDER_POS_Y 36.0
#define EXENSION1 30.0
#define CANVAS_Y 142
#define ANGLE_PER_STEP (360.0/(25600.0))
#define MAX_CANVAS_SIZE_X 360.0
#define MAX_CANVAS_SIZE_Y 360.0

#else

#define CENTER 550.0
#define UPPER_ARM_LENGTH 700.0
#define LOWER_ARM_LENGTH 900.0
#define SHOULDER_DIST 300.0
#define SHOULDER_HEIGHT 100.0
#define LEFT_SHOULDER_POS_X (CENTER - 150.0)
#define RIGHT_SHOULDER_POS_X (CENTER + 150.0)
#define LEFT_SHOULDER_POS_Y 100.0
#define RIGHT_SHOULDER_POS_Y 100.0
#define EXENSION1 72.0
#define CANVAS_Y 330
#define ANGLE_PER_STEP (360.0/(3200.0*50.0))
#define MAX_CANVAS_SIZE_X 1100.0
#define MAX_CANVAS_SIZE_Y 1200.0

#endif

#endif

#endif


