//class Preview//
#include "bool.h"

#ifndef PREVIEW_H
#define PREVIEW_H

typedef struct Preview{
    int retainCount;
    char *type;
    char *imageName;
    unsigned char* imageData;
    int width;
    int height;
    int maxDelay;
    int minDelay;
}Preview;

Preview *Preview_alloc(int width, int height, char *imagename,int maxDelay, int minDelay);
void Preview_setPixel(Preview *self,int x, int y,int delay, bool shouldDraw);
void Preview_save(Preview *self);
void Preview_release(Preview *p);
void Preview_retain(Preview *p);
void Preview_updateSpeed(Preview *self, int maxDelay, int minDelay);

#endif
