//user: rpi - pw: linuxcnc ip: 192.168.0.102
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "sketchy.h"
#include "Model.h"
#include "bool.h"
#include "Config.h"
#include "sketchy-ipc.h"
#include "machine-settings.h"

#ifdef __PI__
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <bcm2835.h>
#include <native/task.h>
#include <native/timer.h>
#endif

#ifndef __PI__
#include "Preview.h"
#endif

#ifdef __PI__
RT_TASK pen_task;
RT_TASK draw_task;
RT_TASK watchdog_task;
#endif

/**

    Jeroen, I guess this is for b+? pin 29 and 31?
    check: https://www.raspberrypi-spy.co.uk/wp-content/uploads/2012/06/Raspberry-Pi-GPIO-Layout-Model-B-Plus-rotated-2700x900.png    

    from bmc2835.h:

    // RPi Version 2, new plug P5
    RPI_V2_GPIO_P5_03     = 28,  ///< Version 2, Pin P5-03
    RPI_V2_GPIO_P5_04     = 29,  ///< Version 2, Pin P5-04
    RPI_V2_GPIO_P5_05     = 30,  ///< Version 2, Pin P5-05
    RPI_V2_GPIO_P5_06     = 31,  ///< Version 2, Pin P5-06

*/


#define PEN_CLOCK RPI_V2_GPIO_P1_03
#define PEN_DIR RPI_V2_GPIO_P1_05
#define RIGHT_CLOCK RPI_V2_GPIO_P1_22 
#define RIGHT_DIR RPI_V2_GPIO_P1_18
#define LEFT_CLOCK RPI_V2_GPIO_P1_13
#define LEFT_DIR RPI_V2_GPIO_P1_15
#define CENTER_CLOCK RPI_V2_GPIO_P1_11
#define CENTER_DIR RPI_V2_GPIO_P1_12
#define SOLENOID RPI_V2_GPIO_P1_16

#define CENTER_LIMIT RPI_V2_GPIO_P1_08
#define LEFT_LIMIT RPI_V2_GPIO_P1_07
#define RIGHT_LIMIT RPI_V2_GPIO_P1_10

StepperMotorDir stepleft = stepperMotorDirNone;
StepperMotorDir stepright = stepperMotorDirNone;
HorizontalMovementDir stepcenter = horizontalMovementDirNone;

StepperMotorDir leftdir = stepperMotorDirNone;
StepperMotorDir rightdir = stepperMotorDirNone;
HorizontalMovementDir centerdir = horizontalMovementDirNone;

SolenoidState solenoidstate = solenoidStateUp;
SolenoidState solenoid = solenoidStateUp;

int penmovestate = 1;
int penmove = 1;

int pen_y = 0;
int pen_max_y = 800;


#ifndef __PI__
FILE *fp;
Preview *PREVIEW;
Preview *PREVIEW_PEN_MOVE;
long stepCounter = 0;
#endif

const char *input_imagename;
int input_threshold;

static bool paused = false;

void sketchy_suspend(){

    if(!paused){
        updateDriverState(driverStatusCodePaused,"","");
        paused = true;
        printf("PAUSED\n");

#ifdef __PI__
        rt_task_suspend(&draw_task);
        rt_task_suspend(&pen_task);
#else
        // on a non xenomai os this busy loop simulates rt_task_suspend
        while(1){
            DriverCommand *cmd = getCommand();
            if(cmd->commandCode != commandCodePause){
                sketchy_resume();
                return;
            }
        }

#endif
    }
}

void sketchy_resume(){

    if(paused){
        Model_resume();
#ifndef __PI__
        Preview_updateSpeed(PREVIEW, Config_maxDelay(), Config_minDelay());
#endif
        updateDriverState(driverSatusCodeBusy,"","");
        paused = false;
#ifdef __PI__
        printf("-rt_task_resume-\n");
        rt_task_resume(&draw_task);
        rt_task_resume(&pen_task);
#else
        printf("RESUME\n");
#endif

    }
}

#ifdef __PI__

void pen_action(){
    while(1){
        rt_task_wait_period(NULL);
        bool shouldDraw = (BOT->penMode == penModeManualDown);
        if(shouldDraw){
            penmove = 0;
        }else{
            penmove = 1;
        }
        if(penmovestate != penmove){
            if(penmove == 0){
                bcm2835_gpio_write(PEN_DIR, HIGH);
                if(pen_y == 0){
                    pen_y = 1;
                }
            }else if(penmove == 1){
                bcm2835_gpio_write(PEN_DIR, LOW);
                if(pen_y == pen_max_y){
                    pen_y = pen_max_y - 1;
                }
            }
            penmovestate = penmove;
        }
        if (pen_y < pen_max_y && pen_y > 0){
            if(penmove == 0){
                pen_y ++;
            }
            if(penmove == 1){
                pen_y --;
            }
            bcm2835_gpio_write(PEN_CLOCK, HIGH);
            rt_task_sleep(100);
            bcm2835_gpio_write(PEN_CLOCK, LOW);
        }
        rt_task_set_periodic(&pen_task, TM_NOW, 500000);
    }
}

// On the raspberry PI / xenomai the watchdog rt_task
// checks if the draw task needs to be resumed
void watch(){
    while(1){
        rt_task_wait_period(NULL);
        if(paused){
            DriverCommand *cmd = getCommand();
            if(cmd->commandCode != commandCodePause){
                sketchy_resume();
            }
        }
        rt_task_set_periodic(&watchdog_task, TM_NOW, 500000);
    }
}

#endif

void executeStep(Step *step){

    bool shouldDraw = (BOT->penMode == penModeManualDown);

#ifdef __PI__

    rt_task_wait_period(NULL);

    stepleft = step->leftengine;
    stepright = step->rightengine;
    stepcenter =  step->horengine;
    if(shouldDraw){
        solenoid = solenoidStateDown;
    }else{
        solenoid = solenoidStateUp;
    }

    if(stepleft != leftdir){

        //FOR MINI blackstripes with no gearboxes HIGH and LOW should be inverted for left only, NOT RIGHT!!
        //the gearboxes are mirrored to make the machine look better
        //so the direction signals have to be inverted
        if(stepleft == stepperMotorDirUp){
            bcm2835_gpio_write(LEFT_DIR, LOW);
        }else if(stepleft == stepperMotorDirDown){
            bcm2835_gpio_write(LEFT_DIR, HIGH);
        }

        leftdir = stepleft;

    }

    if(stepright != rightdir){

        if(stepright == stepperMotorDirUp){
            bcm2835_gpio_write(RIGHT_DIR, HIGH);
        }else if(stepright == stepperMotorDirDown){
            bcm2835_gpio_write(RIGHT_DIR, LOW);
        }

        rightdir = stepright;

    }

    if(stepcenter != centerdir){
 
        if(stepcenter == horizontalMovementDirRight){
                bcm2835_gpio_write(CENTER_DIR, HIGH);
        }else if(stepcenter == horizontalMovementDirLeft){
                bcm2835_gpio_write(CENTER_DIR, LOW);
        }

        centerdir = stepcenter;
    }

    if(solenoidstate != solenoid){

        if(solenoid == solenoidStateUp){
            bcm2835_gpio_write(SOLENOID, HIGH);
        }else if(solenoid == solenoidStateDown){
            bcm2835_gpio_write(SOLENOID, LOW);
        }

        solenoidstate = solenoid;

    }

    // sync the stepper steps //
    if (stepleft != stepperMotorDirNone) {
        bcm2835_gpio_write(LEFT_CLOCK, HIGH);
    }
    if (stepright != stepperMotorDirNone) {
        bcm2835_gpio_write(RIGHT_CLOCK, HIGH);
    }
    if (stepcenter != horizontalMovementDirNone) {
        bcm2835_gpio_write(CENTER_CLOCK, HIGH);
    }

    rt_task_sleep(100);

    if (stepleft != stepperMotorDirNone) {
        bcm2835_gpio_write(LEFT_CLOCK, LOW);
    }
    if (stepright != stepperMotorDirNone) {
        bcm2835_gpio_write(RIGHT_CLOCK, LOW);
    }
    if (stepcenter != horizontalMovementDirNone) {
        bcm2835_gpio_write(CENTER_CLOCK, LOW);
    }

    rt_task_set_periodic(&draw_task, TM_NOW, BOT->delay);

#else

    Point *p = Point_allocWithSteps(BOT->leftsteps,BOT->rightsteps);
    int x = floor(p->x);
    int y = floor(p->y);
    Preview_setPixel(PREVIEW, x , y, BOT->delay, shouldDraw);
    Preview_setPixel(PREVIEW_PEN_MOVE, x , y, BOT->delay, !shouldDraw);
    float c = BOT->centersteps * MOVEMENT_STEP;
    if(stepCounter%500 == 0){
        fprintf (fp, "%f, %f, %f, %i, %i, \n", p->left_angle, p->right_angle, c, shouldDraw, BOT->delay);
    }
    stepCounter ++;
    if(stepCounter%1000000 == 0){
        fflush(fp);
        Preview_save(PREVIEW);
        Preview_save(PREVIEW_PEN_MOVE);
    }
    Point_release(p);

#endif


}


// move to null position until we run into the limit switches
int autoNull(){
    BOT->penMode = penModeManualUp; 
    BOT->delay = 350000; 
    Step *step = Step_alloc(stepperMotorDirUp, stepperMotorDirUp, horizontalMovementDirLeft);

#ifdef __PI__
    int nullingInProgress = 1;
    while(nullingInProgress){
        StepperMotorDir l = stepperMotorDirDown;
        StepperMotorDir r = stepperMotorDirDown;
        HorizontalMovementDir c = horizontalMovementDirLeft;

        int left_inp = bcm2835_gpio_lev(LEFT_LIMIT);
        int right_inp = bcm2835_gpio_lev(RIGHT_LIMIT);
        int center_inp = bcm2835_gpio_lev(CENTER_LIMIT);

        if(left_inp == HIGH){
            l = stepperMotorDirNone;
        }
        if(right_inp == HIGH){
            r = stepperMotorDirNone;
        }
        if(center_inp == HIGH){
            c = horizontalMovementDirNone;
        }

        //printf("l %i r %i c %i\n", left_inp, right_inp, center_inp);

        Step_update(step, l, r, c);
        executeStep(step);

        if(left_inp == HIGH && right_inp == HIGH && center_inp == HIGH){
            nullingInProgress = 0;
        }
    }

    int num_steps = 32000;
    HorizontalMovementDir h = horizontalMovementDirRight;
    while(num_steps--){
        if(num_steps < 32000 - 12800){
        h = horizontalMovementDirNone;
    }
        Step_update(step, stepperMotorDirUp, stepperMotorDirUp, h);
        executeStep(step);
    }

#endif

    Step_release(step);
#ifdef __PI__
    alarm(1);
#endif
    return 0;
}


void catch_signal(int sig)
{
}

int run(void (*executeMotion)()){

    Model_createInstance();
    Model_setExecuteStepCallback(executeStep);
    Model_logState();

#ifdef __PI__

    signal(SIGTERM, catch_signal);
    signal(SIGINT, catch_signal);
    signal(SIGALRM, catch_signal);

    /* Avoids memory swapping for this program */
    mlockall(MCL_CURRENT|MCL_FUTURE);

    if (!bcm2835_init()){
        printf("error\n");
        //return 1;
    }


    bcm2835_gpio_fsel(PEN_CLOCK, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(PEN_DIR, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(RIGHT_CLOCK, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(RIGHT_DIR, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(LEFT_CLOCK, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(LEFT_DIR, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(CENTER_CLOCK, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(CENTER_DIR, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(SOLENOID, BCM2835_GPIO_FSEL_OUTP);

    bcm2835_gpio_fsel(CENTER_LIMIT, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(LEFT_LIMIT, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(RIGHT_LIMIT, BCM2835_GPIO_FSEL_INPT);

    bcm2835_gpio_set_pud(CENTER_LIMIT, BCM2835_GPIO_PUD_DOWN);
    bcm2835_gpio_set_pud(LEFT_LIMIT, BCM2835_GPIO_PUD_DOWN);
    bcm2835_gpio_set_pud(RIGHT_LIMIT, BCM2835_GPIO_PUD_DOWN);

    bcm2835_gpio_write(SOLENOID, HIGH);
    rt_task_set_periodic(&draw_task, TM_NOW, BOT->delay);
    rt_task_set_periodic(&pen_task, TM_NOW, 50000);
    rt_task_set_periodic(&watchdog_task, TM_NOW, 500000);

    /*
     * Arguments: &task,
     *            name,
     *            stack size (0=default),
     *            priority,
     *            mode (FPU, start suspended, ...)
     */
    rt_task_create(&draw_task, "printerbot", 0, 99, 0);
    rt_task_create(&pen_task, "penlift", 0, 99, 0);
    rt_task_create(&watchdog_task, "watchdog", 0, 99, 0);
    /*
     * Arguments: &task,
     *            task function,
     *            function argument
     */
    rt_task_start(&draw_task, executeMotion, NULL);
    rt_task_start(&pen_task, &pen_action, NULL);
    rt_task_start(&watchdog_task, &watch, NULL);

    pause();

    rt_task_delete(&draw_task);
    rt_task_delete(&pen_task);
    rt_task_delete(&watchdog_task);

    bcm2835_gpio_write(SOLENOID, HIGH);


#else

    fp = fopen("movement.js","w");
    fprintf(fp, "var DATA = [\n");
    PREVIEW = Preview_alloc((int)MAX_CANVAS_SIZE_X,(int)MAX_CANVAS_SIZE_Y,"preview_image.png",Config_maxDelay(),Config_minDelay());
    PREVIEW_PEN_MOVE = Preview_alloc((int)MAX_CANVAS_SIZE_X,(int)MAX_CANVAS_SIZE_Y,"pen_move_image.png",Config_maxDelay(),Config_minMoveDelay());

    report_memory(1);
    executeMotion();
    Preview_save(PREVIEW);
    Preview_release(PREVIEW);
    Preview_save(PREVIEW_PEN_MOVE);
    Preview_release(PREVIEW_PEN_MOVE);
    fprintf(fp, "]\n");
    fclose(fp);

#endif

    Model_release();

    return 0;

}



