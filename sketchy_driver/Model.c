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

/**
 
 def prepare_axis(l, _max, left_skips):
    if l == _max:
        lmr = 1
        lp = 0
        lsp = 1
        lmax = _max
    else:
        if left_skips > 0 and l > 0:
            if left_skips >= l:
                lmr = 1
                lp = 0
                lsp = int(_max / l)
                lmax = l
            else:
                lmr = 0
                lp = 1
                lsp = int(_max / left_skips)
                lmax = left_skips
    return lmr, lp, lsp, lmax

 */

void Model_prepareAxis(int *lmr, int *lp, int *lsp, int *lmax, 
                       int l, int _max, int left_skips,
                       int value, int spacing_value) {
    if(l == _max){
        *lmr = value;
        *lp = spacing_value;
        *lsp = 1;
        *lmax = _max;
    }else if(l == 0){
        *lmr = spacing_value;
        *lp = value;
        *lsp = 1;
        *lmax = _max;
    }else{
        if(left_skips > 0 && l > 0){
            if(left_skips >= l){
                *lmr = value;
                *lp = spacing_value;
                *lsp = (int)(_max / l);
                *lmax = l;
            }else{
                *lmr = spacing_value;
                *lp = value;
                *lsp = (int)(_max / left_skips);
                *lmax = left_skips;
            }
        }
    }
    return;
}

void Model_generateSteps(Point *to, float center){

    //Point_log(to);

    int delta_steps_center = (int)round(center / MOVEMENT_STEP) - BOT->centersteps;
    int delta_steps_left = to->left_steps - BOT->leftsteps;
    int delta_steps_right = to->right_steps - BOT->rightsteps;

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

    int leftSkips = largest - abs(delta_steps_left);
    int rightSkips = largest - abs(delta_steps_right);
    int centerSkips = largest - abs(delta_steps_center);

    int lmr; int lp; int lsp; int lmax;
    Model_prepareAxis(&lmr, &lp, &lsp, &lmax, abs(delta_steps_left), largest, leftSkips, stepperdir_left, stepperMotorDirNone);

    int rmr; int rp; int rsp; int rmax;
    Model_prepareAxis(&rmr, &rp, &rsp, &rmax, abs(delta_steps_right), largest, rightSkips, stepperdir_right, stepperMotorDirNone);

    int cmr; int cp; int csp; int cmax;
    Model_prepareAxis(&cmr, &cp, &csp, &cmax, abs(delta_steps_center), largest, centerSkips, stepperdir_center, horizontalMovementDirNone);

    int lc = 0; int rc = 0; int cc = 0;

    Step *step = Step_alloc(stepperMotorDirNone, stepperMotorDirNone, horizontalMovementDirNone);

    int i = 0;
    for(i = 0; i < largest; i++){

        int _l = 0; int _r = 0; int _c = 0;

        if (i%lsp == 0 && lc < lmax){
            _l = lmr;
        }else{
            _l = lp;
        }
        if (i%rsp == 0 && rc < rmax){
            _r = rmr;
        }else{
            _r = rp;
        }
        if (i%csp == 0 && cc < cmax){
            _c = cmr;
        }else{
            _c = cp;
        }

        if (_l == stepperdir_left){
            lc ++;
        }
        if (_r == stepperdir_right){
            rc ++;
        }
        if (_c == stepperdir_center){
            cc ++;
        }

        Step_update(step, _l, _r, _c);
        //printf("%i %i %i\n",_l,_r,_c);
        Model_addStep(step->leftengine, step->rightengine, step->horengine);
        BOT->executeStepCallback(step);
    }
    //printf("- - - - - - \n");
    Step_release(step);

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
    //Model_moveTo(BOT->home);
    Model_computeSegments(BOT->home->x, BOT->home->y, 0.0, 1);
    Model_setCenter(0.0);
    Point_updateWithXY(BOT->currentLocation, BOT->home->x, BOT->home->y);
}

void Model_null(){
    printf("nulling...\n");
    int delta_steps_center = 0 - BOT->centersteps;
    int delta_steps_left = 0 - BOT->leftsteps;
    int delta_steps_right = 0 - BOT->rightsteps;
    printf("l %i r %i c %i \n", delta_steps_left, delta_steps_right, delta_steps_center);
    Point *p = Point_allocWithSteps(0,0);
    Model_generateSteps(p, 0);
    Point_release(p);
    Point_updateWithXY(BOT->currentLocation, p->x, p->y);
    Model_setCenter(0.0);
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
