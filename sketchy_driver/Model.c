#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include "Model.h"
#include "FSObject.h"
#include "Point.h"
#include "SpeedManager.h"
#include "Step.h"
#include "machine-settings.h"
#include "Config.h"
#include "sketchy-ipc.h"
#include "bool.h"

#ifdef __APPLE__
#include <mach/mach.h>
#endif

void report_memory(int id) {

#ifdef __APPLE__

    struct task_basic_info info;
    mach_msg_type_number_t size = sizeof(info);

    kern_return_t kerr = task_info(mach_task_self(),
                                   TASK_BASIC_INFO,
                                   (task_info_t)&info,
                                   &size);

    if( kerr == KERN_SUCCESS ) {
        printf("ID %i Memory in use (in MBs): %f\n", id,(info.resident_size/1024.0)/1024.0);
    } else {
        printf("Error with task_info(): %s\n", mach_error_string(kerr));
    }

#endif

}

void log_time(){

    time_t timer;
    char buffer[26];
    struct tm* tm_info;

    time(&timer);
    tm_info = localtime(&timer);

    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    puts(buffer);

}

int Model_getCenter(){
    return BOT->center;
}

void Model_setCenter(float newCenter){
    float diff = newCenter - BOT->center;
    BOT->center = newCenter;
    BOT->currentLocation->x += diff;
}

int Model_getLeftShoulderX(){
    return BOT->center + LEFT_SHOULDER_OFFSET;
}

int Model_getRightShoulderX(){
    return BOT->center + RIGHT_SHOULDER_OFFSET;
}

void Model_resume(){
    SpeedManager_resume(BOT->speedManager);
}

void Model_addStep(int left, int right, int center){

    if(left == stepperMotorDirUp){
        BOT->leftsteps ++;
    }else if(left == stepperMotorDirDown){
        BOT->leftsteps --;
    }

    if(right == stepperMotorDirUp){
        BOT->rightsteps ++;
    }else if(right == stepperMotorDirDown){
        BOT->rightsteps --;
    }

    if(center == horizontalMovementDirRight){
        BOT->centersteps ++;
    }else if(center == horizontalMovementDirLeft){
        BOT->centersteps --;
    }

    BOT->center  = BOT->centersteps * MOVEMENT_STEP;

}

void Model_createInstance(){
    BOT = (BotState *) malloc(sizeof(BotState));
    BOT->home = Point_allocWithSteps(0 ,0);
    BOT->currentLocation = Point_allocWithSteps(0 ,0);
    BOT->leftsteps = BOT->home->left_steps;
    BOT->rightsteps = BOT->home->right_steps;
    BOT->centersteps = 0;
    BOT->retainCount = 1;
    BOT->center = 0.0;
    BOT->delay = Config_maxDelay();
    BOT->type = "BotState";
    BOT->penMode = penModeManualUp;

    sm = SpeedManager_alloc();
    BOT->speedManager = sm;
    SpeedManager_setCallback(sm,SpeedManager_callback);

    Model_toPoint = Point_allocWithSteps(0 ,0);
}

void Model_setPenMode(PenMode mode){
    BOT->scheduledPenMode = mode;
}

void Model_logState(){
    printf("##############################################\n");
    printf("# MACHINE STATE leftsteps %i rightsteps %i centersteps %i\n", BOT->leftsteps, BOT->rightsteps, BOT->centersteps);
    printf("# current location x:%f y:%f center:%f\n", BOT->currentLocation->x, BOT->currentLocation->y, BOT->center);
    printf("##############################################\n");
}

void Model_release(){

    BOT->speedManager = NULL;
    SpeedManager_release(sm);
    Point_release(Model_toPoint);

    report_memory(2);
    Model_logState();

    BOT->retainCount --;
    if(BOT->retainCount == 0){
        Point_release(BOT->currentLocation);
        Point_release(BOT->home);
        free(BOT);
    }
}

void Model_retain(){
    FSObject_retain(BOT);
}

void Model_generateSteps(Point *to){

    //Point_log(to);

    int delta_steps_left = to->left_steps - BOT->leftsteps;
    int delta_steps_right = to->right_steps - BOT->rightsteps;

    int largest = MAX(abs(delta_steps_left),abs(delta_steps_right));
    int smallest = MIN(abs(delta_steps_left),abs(delta_steps_right));

    //printf("LEFT %i RIGHT %i MAX %i \n\n",delta_steps_left,delta_steps_right,largest);

    StepperMotorDir stepperdir_left = stepperMotorDirNone;
    StepperMotorDir stepperdir_right = stepperMotorDirNone;

    if(delta_steps_left < 0){
        stepperdir_left = stepperMotorDirDown;
    }else if(delta_steps_left > 0){
        stepperdir_left = stepperMotorDirUp;
    }else{
        stepperdir_left = stepperMotorDirNone;
    }

    if(delta_steps_right < 0){
        stepperdir_right = stepperMotorDirDown;
    }else if(delta_steps_right > 0){
        stepperdir_right = stepperMotorDirUp;
    }else{
        stepperdir_right = stepperMotorDirNone;
    }

    Step *step = Step_alloc(stepperMotorDirNone, stepperMotorDirNone, horizontalMovementDirNone);

//    printf("INPUT largest %i smallest %i\n\n",largest,smallest);

    int switchmode = 0;
    float factor = (float)largest / (float)smallest;

    int skip = round(factor);

    if(factor < 2.0 && factor > 1.0){
        skip = round((float)largest / (float)(largest-smallest));
        switchmode = 1;
    }

    int insertcount = 0;
    int largestcount = 0;

    StepperMotorDir skipperValue;

    if(abs(delta_steps_left) > abs(delta_steps_right)){
        skipperValue = stepperdir_right;
    }else{
        skipperValue = stepperdir_left;
    }

    int i = 0;
    for(i = 0; i< largest; i++){

        StepperMotorDir skipper;

        if(switchmode){

            skipper = skipperValue;
            if(i%skip == 0 && insertcount < (largest-smallest)){
                skipper = stepperMotorDirNone;
                insertcount ++;
            }

        }else{

            skipper = stepperMotorDirNone;
            if(i%skip == 0 && insertcount < smallest){
                skipper = skipperValue;
                insertcount ++;
            }

        }

        largestcount ++;

        HorizontalMovementDir d = horizontalMovementDirNone;
        if(to->x > BOT->center + BOT_REPOSITION_THRESHOLD){
            d = horizontalMovementDirRight;
        }else if(to->x < BOT->center - BOT_REPOSITION_THRESHOLD){
            d = horizontalMovementDirLeft;
        }

        if(abs(delta_steps_left) > abs(delta_steps_right)){
            Step_update(step, stepperdir_left, skipper, d);
        }else{
            Step_update(step, skipper, stepperdir_right, d);
        }

        Model_addStep(step->leftengine, step->rightengine, step->horengine);

        BOT->executeStepCallback(step);

    }

//    if(switchmode){
//        printf("skip %i largest %i smallest %i\n\n",skip,largestcount,largest-insertcount);
//    }else{
//        printf("skip %i largest %i smallest %i\n\n",skip,largestcount,insertcount);
//    }


    Step_release(step);

}

bool willDrawForLevelAtPoint(){
    return (BOT->scheduledPenMode == penModeManualDown);
}

void Model_computeSegments(float _x, float _y){

    float deltaX = BOT->currentLocation->x - _x;
    float deltaY = BOT->currentLocation->y - _y;

    float length = sqrt(deltaX*deltaX + deltaY*deltaY);

    int numsteps = round(length/LINE_SEGMENT_SIZE_MM);
    float numspaces = (float)numsteps;

    float xstep = deltaX/numspaces;
    float ystep = deltaY/numspaces;

    float x = BOT->currentLocation->x;
    float y = BOT->currentLocation->y;

    int i=0;
    for (i=0; i < numsteps-1; i++){
        x = x - xstep;
        y = y - ystep;
        bool willDraw = willDrawForLevelAtPoint();
        SpeedManager_append(sm,x,y,BOT->scheduledPenMode,willDraw);
    }

    bool willDraw = willDrawForLevelAtPoint();
    SpeedManager_append(sm,_x,_y,BOT->scheduledPenMode,willDraw);

}

void Model_finish(){
    SpeedManager_finish(sm);
}

void SpeedManager_callback(float x, float y, int delay, int cursor, int penMode){
    //printf("callback x %f y %f delay %i \n",x,y,delay);
    BOT->penMode = penMode;
    BOT->delay = delay;
    Point_updateWithXY(Model_toPoint, x, y);
    Model_generateSteps(Model_toPoint);
}

void Model_moveHome(){
    printf("homing...\n");
    //Model_moveTo(BOT->home);
    Model_computeSegments(BOT->home->x, BOT->home->y);
    Point_updateWithXY(BOT->currentLocation, BOT->home->x, BOT->home->y);
}

void Model_moveTo(float x, float y){

    int w = Config_getCanvasWidth();
    int h = Config_getCanvasHeight();

    //Point_updateWithXY(dest, dest->x, dest->y + (MAX_CANVAS_SIZE_Y/2.0 - h/2.0));

    Model_computeSegments(x,y);
    Point_updateWithXY(BOT->currentLocation,x,y);
}

void Model_setExecuteStepCallback(void (*executeStepCallback)(Step *step)){
    BOT->executeStepCallback = executeStepCallback;
}
