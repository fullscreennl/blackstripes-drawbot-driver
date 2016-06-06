//class SpeedManager//
#ifndef SPEEDMANAGER_H
#define SPEEDMANAGER_H

#define LOOKAHEAD_IN_MM 100

typedef struct PathSegment
{
    float direction;
    struct PathSegment *next;
    float x;
    float y;
    int penMode;
    int solenoidState;
}PathSegment;


typedef struct SpeedManager{
    int retainCount;
    char *type;
    int queueLength;
    int length;
    PathSegment* bottom;
    PathSegment* top;
    float max;
    float currentDirection;
    float currentX;
    float currentY;
    void (*executeCallback)(float x, float y, int delay,int cursor,int penMode);
    int delay;
    int targetDelay;
    int delayStep;
    int delayStepDraw;
    int delayStepMove;
    float delayPerDegree;
    float delayPerDegreeMove;
    int usePenChangeInLookAhead;
}SpeedManager;

SpeedManager *SpeedManager_alloc();
void SpeedManager_append(SpeedManager *sm,float x,float y,int penMode,int solenoidState);
void SpeedManager_setCallback(SpeedManager *sm,void (*executeCallback)(float x,float y, int delay,int cursor,int penMode));
void SpeedManager_resume(SpeedManager *sm);
void SpeedManager_finish(SpeedManager *sm);
void SpeedManager_release(SpeedManager *sm);
void SpeedManager_retain(SpeedManager *sm);
void SpeedManager_reduceQueue(SpeedManager *sm);

#endif


