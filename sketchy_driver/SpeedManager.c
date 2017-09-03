#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include "FSObject.h"
#include "SpeedManager.h"
#include "machine-settings.h"
#include "Config.h"
#include "sketchy-ipc.h"
#include "bool.h"
#include "sketchy.h"

static bool pausingInitialized = false;
static int easeOutDelay = 0;

SpeedManager *SpeedManager_alloc()
{
    SpeedManager *sm = (SpeedManager *) malloc(sizeof(SpeedManager));

    Point *home = Point_allocWithSteps(0 ,0);

    sm->top = (PathSegment *) malloc(sizeof(PathSegment));
    sm->top->direction = 0.0;
    sm->top->next = NULL;
    sm->top->x = home->x;
    sm->top->y = home->y;

    sm->bottom = sm->top;
    sm->queueLength = 0;
    sm->length = Config_getLookaheadMM();
    sm->retainCount = 1;
    sm->type = "SpeedManager";
    sm->max = 0;
    sm->currentX = home->x;
    sm->currentY = home->y;
    sm->currentDirection = 0.0;
    sm->delay = Config_maxDelay();
    sm->targetDelay = Config_maxDelay();
    sm->delayPerDegree = (sm->delay - Config_minDelay()) / 180.0;
    sm->delayPerDegreeMove = (sm->delay - Config_minMoveDelay()) / 180.0;
    sm->delayStepDraw = fabs((Config_maxDelay() - Config_minDelay()) / sm->length);
    sm->delayStepMove = fabs((Config_maxDelay() - Config_minMoveDelay()) / sm->length);
    sm->delayStep = sm->delayStepDraw;
    sm->usePenChangeInLookAhead = Config_usePenChangeInLookAhead();

    Point_release(home);

    return sm;
}

void SpeedManager_resume(SpeedManager *sm){
    Config_reload();
    sm->length = Config_getLookaheadMM();
    sm->delayPerDegree = (Config_maxDelay() - Config_minDelay()) / 180.0;
    sm->delayPerDegreeMove = (Config_maxDelay() - Config_minMoveDelay()) / 180.0;
    sm->delayStepDraw = fabs((Config_maxDelay() - Config_minDelay()) / sm->length);
    sm->delayStepMove = fabs((Config_maxDelay() - Config_minMoveDelay()) / sm->length);
    sm->delayStep = sm->delayStepDraw;
    sm->usePenChangeInLookAhead = Config_usePenChangeInLookAhead();
    SpeedManager_reduceQueue(sm);
}

void SpeedManager_copmuteDelay(SpeedManager *sm){
    if(fabs(sm->targetDelay - sm->delay) < sm->delayStep){
        sm->delay = sm->targetDelay;
    }else if(sm->targetDelay > sm->delay){
        sm->delay += sm->delayStep;
    }else if(sm->targetDelay < sm->delay){
        sm->delay -= sm->delayStep;
    }
}

void SpeedManager_log(SpeedManager *sm){
    if(sm->bottom == sm->top){
        printf("SpeedManager state EMPTY \n");
        return;
    }
    int i=0;
    PathSegment *curr = sm->bottom;
    while(curr){
        printf("%i -> x%f y:%f -> direction: %f \n",i,curr->x,curr->y,curr->direction);
        curr = curr->next;
        i++;
    }
    printf("- - - - - - \n");
    printf("MAX %f \n",sm->max);
    printf("- - - - - - \n");
}

void SpeedManager_compute(SpeedManager *sm){

    float max = 0;
    PathSegment *curr = sm->bottom;
    int penChangeAhead = 0;
    int penUpAhead = 0;
    int penDownAhead = 0;
    int solenoidState = curr->solenoidState;

    while(curr){
        if(solenoidState != curr->solenoidState){
            penChangeAhead = 1;
            if(solenoidState == 0){
                penUpAhead = 0;
                penDownAhead = 1;
                sm->delayStep = sm->delayStepMove;
            }else{
                penUpAhead = 1;
                penDownAhead = 0;
                sm->delayStep = sm->delayStepDraw;
            }
        } 
        if(fabs(curr->direction) > max){
            max = fabs(curr->direction);
        }
        curr = curr->next;
    }

    if(penChangeAhead && sm->usePenChangeInLookAhead){
        sm->targetDelay = Config_maxDelay();
    }else if(max != sm->max){
        sm->max = max;
        if(solenoidState == 0){
            sm->targetDelay = Config_minMoveDelay() + sm->max * sm->delayPerDegreeMove;
        }else{
            sm->targetDelay = Config_minDelay() + sm->max * sm->delayPerDegree;
        }
    }
}

void SpeedManager_append(SpeedManager *sm, float x, float y, float c, int penMode, int solenoidState){
    
    float dir = atan2(sm->currentY - y, sm->currentX - x);

    PathSegment *seg = (PathSegment *) malloc(sizeof(PathSegment));
    float tmp = fabs((dir - sm->currentDirection) * DEG);
    if(tmp > 180){
        tmp = tmp - 360.0;
    }
    seg->direction = tmp;
    seg->next = NULL;
    seg->x = x;
    seg->y = y;
    seg->c = c;
    seg->penMode = penMode;
    seg->solenoidState = solenoidState;

    sm->top->next = seg;
    sm->top = seg;

    sm->currentDirection = dir;
    sm->currentX = x;
    sm->currentY = y;

    if(sm->queueLength >= sm->length-1){

        DriverCommand *cmd = getCommand();

        SpeedManager_copmuteDelay(sm);
        int computedDelay = sm->delay;

        if(cmd->commandCode == commandCodePause){
            if(!pausingInitialized){
                easeOutDelay = sm->delay;
                pausingInitialized = true;
            }
            easeOutDelay += 5000;
            computedDelay = easeOutDelay;
            if(easeOutDelay > Config_maxDelay()){
                sm->delay = easeOutDelay;
                sketchy_suspend();
            }
        }else{
            pausingInitialized = false;
            easeOutDelay = 0;
        }

        sm->executeCallback(sm->bottom->x,sm->bottom->y,sm->bottom->c,computedDelay,sm->queueLength,sm->bottom->penMode);
        PathSegment *newBottom = sm->bottom->next;
        free(sm->bottom);
        sm->bottom = newBottom;
        SpeedManager_compute(sm);
    }else{
        sm->queueLength ++;
    }

}

void SpeedManager_setCallback(SpeedManager *sm,void (*executeCallback)(float x, float y, float c, int delay, int cursor, int penMode)){
    sm->executeCallback = executeCallback;
}

void SpeedManager_release(SpeedManager *sm){
    sm->retainCount --;
    if(sm->retainCount == 0){
        PathSegment *p;
        PathSegment *curr = sm->bottom;
        while(curr){
            p = curr->next;
            free(curr);
            curr = p;
        }
        free(sm);
    }
}

void SpeedManager_reduceQueue(SpeedManager *sm){
    PathSegment *curr = sm->bottom;
    while(sm->queueLength > sm->length-1){
        SpeedManager_copmuteDelay(sm);
        int computedDelay = sm->delay;
        sm->executeCallback(sm->bottom->x, sm->bottom->y, sm->bottom->c, computedDelay, sm->queueLength, sm->bottom->penMode);
        PathSegment *newBottom = sm->bottom->next;
        free(sm->bottom);
        sm->bottom = newBottom;
        SpeedManager_compute(sm);
        sm->queueLength --;
    }
}

void SpeedManager_finish(SpeedManager *sm){
    PathSegment *curr = sm->bottom;
    while(curr){
        SpeedManager_copmuteDelay(sm);
        sm->executeCallback(curr->x,curr->y,curr->c,sm->delay,sm->queueLength,curr->penMode);
        curr = curr->next;
    }

#ifdef __PI__
    alarm(1);
#endif

}

void SpeedManager_retain(SpeedManager *sm){
    FSObject_retain(sm);
}
