#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
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
    return BOT->currentCenter;
}

void Model_setCenter(float newCenter){
    //float delta = newCenter - BOT->currentCenter;
    BOT->currentCenter = newCenter;
    //BOT->currentLocation->x += delta;
}

int Model_getLeftShoulderX(){
    return BOT->currentCenter + LEFT_SHOULDER_OFFSET;
}

int Model_getRightShoulderX(){
    return BOT->currentCenter + RIGHT_SHOULDER_OFFSET;
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

    //BOT->currentCenter = BOT->centersteps * MOVEMENT_STEP;

}

void Model_createInstance(){
    BOT = (BotState *) malloc(sizeof(BotState));
    BOT->home = Point_allocWithSteps(0 ,0);
    BOT->currentLocation = Point_allocWithSteps(0 ,0);
    BOT->currentCenter = 0.0;
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

int *createAxis(int _steps, int _space, int max_iter, int value){
    int l = _space;
    int numsteps = _steps;
    int iterations = 0;
    int i;
    int *steps = (int *) malloc(l * sizeof(int));
    memset(steps, 0, l * sizeof(int));
    
    for(i=0; i< l; i++){
        steps[i] = 1;
    }

    if(_steps == 0){
        return steps;
    }

    float sp = l/(float)numsteps;
    int insert_count = 0;
    for(i=0; i< l; i+= (int)ceil(sp)){
        if(steps[i] == 1){
            steps[i] = value;
            insert_count ++;
        }
    }
    numsteps = numsteps - insert_count;

    int num_freeindexes = l - insert_count;
    int *freeindexes = (int *) malloc(num_freeindexes * sizeof(int));
    memset(freeindexes, 0, num_freeindexes * sizeof(int));

    while(numsteps > 0){
        num_freeindexes = l - insert_count;

        int index_counter = 0;
        for(i=0; i<l; i++){
            if(steps[i] == 1){
                freeindexes[index_counter] = i;
                index_counter ++;
            }
        }

        sp = num_freeindexes/(float)numsteps;
        int local_insert_count = 0;
        int stride;
        if(iterations >= max_iter){
           stride = (int)floor(sp);
        }else{
           stride = (int)ceil(sp);
        }
        for(i=0; i< num_freeindexes; i += stride){
            if(local_insert_count < numsteps){
                int ind = freeindexes[i];
                steps[ind] = value;
                insert_count ++;
                local_insert_count ++;
            }
        }
        numsteps = numsteps - local_insert_count;
        iterations ++;
    }
    free(freeindexes);
    return steps;
}

void Model__generateSteps(int delta_steps_center, int delta_steps_left, int delta_steps_right){

    int largest = MAX(abs(delta_steps_left),abs(delta_steps_right));
    largest = MAX(largest, abs(delta_steps_center));

    StepperMotorDir stepperdir_left = stepperMotorDirNone;
    StepperMotorDir stepperdir_right = stepperMotorDirNone;
    HorizontalMovementDir stepperdir_center = horizontalMovementDirNone;

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
 
    if(delta_steps_center < 0){
        stepperdir_center = horizontalMovementDirLeft;
    }else if(delta_steps_center > 0){
        stepperdir_center = horizontalMovementDirRight;
    }else{
        stepperdir_center = horizontalMovementDirNone;
    }

    int *left_axis = createAxis(abs(delta_steps_left), largest, 10, stepperdir_left);
    int *right_axis = createAxis(abs(delta_steps_right), largest, 10, stepperdir_right);
    int *center_axis = createAxis(abs(delta_steps_center), largest, 10, stepperdir_center);

    Step *step = Step_alloc(stepperMotorDirNone, stepperMotorDirNone, horizontalMovementDirNone);

    int i = 0;
    for(i = 0; i < largest; i++){
        // printf("l %i  r %i c %i \n", left_axis[i], right_axis[i], center_axis[i]);
	Step_update(step, left_axis[i], right_axis[i], center_axis[i]);
        Model_addStep(step->leftengine, step->rightengine, step->horengine);
        BOT->executeStepCallback(step);
    }
    // printf("- - - \n");
    Step_release(step);
	
    free(left_axis);
    free(right_axis);
    free(center_axis);
}


void Model_generateSteps(Point *to, float center){
    int delta_steps_center = (int)round(center / MOVEMENT_STEP) - BOT->centersteps;
    int delta_steps_left = to->left_steps - BOT->leftsteps;
    int delta_steps_right = to->right_steps - BOT->rightsteps;
    Model__generateSteps(delta_steps_center, delta_steps_left, delta_steps_right);
}


bool willDrawForLevelAtPoint(){
    return (BOT->scheduledPenMode == penModeManualDown);
}

void Model_computeSegments(float _x, float _y, float _c, int headMovement){

    float deltaX = BOT->currentLocation->x - _x;
    float deltaY = BOT->currentLocation->y - _y;
    float deltaC = BOT->currentCenter - _c;

    float length = sqrt(deltaX*deltaX + deltaY*deltaY);

    int numsteps = round(length/LINE_SEGMENT_SIZE_MM);
    float numspaces = (float)numsteps;

    float xstep = deltaX/numspaces;
    float ystep = deltaY/numspaces;
    float cstep = deltaC/numspaces;

    float x = BOT->currentLocation->x;
    float y = BOT->currentLocation->y;
    float c = BOT->currentCenter;

    int i=0;
    for (i=0; i < numsteps-1; i++){
        x = x - xstep;
        y = y - ystep;
        c = c - cstep;
        bool willDraw = willDrawForLevelAtPoint();
        int startMovement = (i == 0 && headMovement);
        SpeedManager_append(sm,x,y,c,BOT->scheduledPenMode,willDraw, startMovement);
    }

    bool willDraw = willDrawForLevelAtPoint();
    SpeedManager_append(sm,_x,_y,_c,BOT->scheduledPenMode,willDraw, headMovement);

}

void Model_finish(){
    SpeedManager_finish(sm);
}

void SpeedManager_callback(float x, float y, float c, int delay, int cursor, int penMode){
    //printf("callback x %f y %f c %f delay %i \n",x,y,c,delay);
    BOT->penMode = penMode;
    BOT->delay = delay;
    Model_setCenter(c);
    Point_updateWithXY(Model_toPoint, x, y);
    Model_generateSteps(Model_toPoint, c);
}

void Model_moveHome(){
    printf("homing...\n");
    Model_computeSegments(BOT->home->x, BOT->home->y, 0.0, 1);
    Model_setCenter(0.0);
    Point_updateWithXY(BOT->currentLocation, BOT->home->x, BOT->home->y);
}

void Model_null(){
    printf("nulling...\n");
    int delta_steps_center = 0 - BOT->centersteps;
    int delta_steps_left = 0 - BOT->leftsteps;
    int delta_steps_right = 0 - BOT->rightsteps;
    Model__generateSteps(delta_steps_center, delta_steps_left, delta_steps_right);
#ifdef __PI__
    alarm(1);
#endif
}

void Model_moveTo(float x, float y){

    float c = Point_needsPositionUpdateWith(x, y);
    int shouldUpdate = Point_shouldPositionUpdateWith(x, y);

    Model_computeSegments(x, y, c, shouldUpdate);

    Model_setCenter(c);
    Point_updateWithXY(BOT->currentLocation,x,y);
}

void Model_setExecuteStepCallback(void (*executeStepCallback)(Step *step)){
    BOT->executeStepCallback = executeStepCallback;
}
