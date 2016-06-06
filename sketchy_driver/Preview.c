#include <stdio.h>
#include <stdlib.h>
#include "machine-settings.h"
#include "Preview.h"
#include "FSObject.h"
#include "Config.h"
#include "bool.h"

#include "lodepng/lodepng.h"

Preview *Preview_alloc(int width, int height, char *imagename,int maxDelay, int minDelay){
    Preview *p = (Preview *) malloc(sizeof(Preview));
    p->width = width;
    p->height = height;
    p->maxDelay = maxDelay;
    p->minDelay = minDelay;
    p->imageName = imagename;
    p->imageData = malloc(width * height * 4);
    p->retainCount = 1;
    p->type = "Preview";
    return p;
}

void Preview_updateSpeed(Preview *self, int maxDelay, int minDelay){
    self->maxDelay = maxDelay;
    self->minDelay = minDelay;
}

void Preview_setPixel(Preview *self,int x, int y,int delay, bool shouldDraw){

    if(!shouldDraw){
        return;
    }

    if(x > self->width-1 || y > self->height-1 || x < 0 || y < 0){
        //printf("out of bounds x %i y %i\n",x,y);
        return;
    }

    int delayProp = Config_maxDelay() - delay;
    int bandWidth = Config_maxDelay() - self->minDelay;
    float perc = (float)delayProp/bandWidth;

    int g = 255.0;
    int r = MIN(255.0,255.0 * perc * 2.0);
    if (perc > 0.5){
        g = (1 - perc) * 2.0 * 255;
    }else{
        g = 255.0;
    }


    self->imageData[4 * self->width * y + 4 * x + 0] = r;
    self->imageData[4 * self->width * y + 4 * x + 1] = g;
    if (! shouldDraw){
        self->imageData[4 * self->width * y + 4 * x + 2] = 255;
    }else{
        self->imageData[4 * self->width * y + 4 * x + 2] = 0;
    }
    self->imageData[4 * self->width * y + 4 * x + 3] = 255;

}

void Preview_save(Preview *self){
    unsigned error = lodepng_encode32_file(self->imageName, self->imageData, self->width, self->height);
    if(error) printf("error %u: %s\n", error, lodepng_error_text(error));
 }

void Preview_release(Preview *self){
    self->retainCount --;
    if(self->retainCount == 0){
        free(self->imageData);
        free(self);
    }
}

void Preview_retain(Preview *self){
    FSObject_retain(self);
}

