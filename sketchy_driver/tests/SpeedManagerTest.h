#include <assert.h>
#include "../SpeedManager.h"
#include "../Config.h"
#include "../machine-settings.h"


#ifndef SPEED_MANAGER_TEST_H
#define SPEED_MANAGER_TEST_H

static int maxDelay;
static int minDelay;

static int testLineLength = 400;

void SpeedManager_log(SpeedManager *sm);

void SpeedManager_testCallback(float x, float y, int delay,int cursor,int penMode){
    int translatedCursor = cursor-LOOKAHEAD_IN_MM+1;
    if(translatedCursor == testLineLength){
        assert(delay == maxDelay);
    }
    if(translatedCursor-1 == LOOKAHEAD_IN_MM){
        assert(delay == minDelay);
    }
    //printf("callback x %f y %f delay %i - cursor %i\n",x,y,delay,cursor-LOOKAHEAD_IN_MM+1);
}


//test 180 degree turn
//should slowdown to speed with maxdelay at the turning point
//should be up to speed with minDelay after LOOKAHEAD_IN_MM
void SpeedManager_test(){

    maxDelay = Config_maxDelay();
    minDelay = Config_minDelay();

    int penMode = 1;
    int solenoidState = 1;
    SpeedManager *sm = SpeedManager_alloc();
    SpeedManager_setCallback(sm,SpeedManager_testCallback);

    //SpeedManager_log(sm);
    Point *home = Point_allocWithSteps(0 ,0);
    float x = home->x;
    float y = home->y;
    Point_release(home);

    int i;
    for(i=0;i<testLineLength;i++){
        y ++;
        SpeedManager_append(sm,x,y,penMode,solenoidState);
    }

    //printf("- - - - - - - -\n");

    for(i=0;i<testLineLength;i++){
        y --;
        SpeedManager_append(sm,x,y,penMode,solenoidState);
    }

    SpeedManager_finish(sm);
    SpeedManager_release(sm);

}

#endif
