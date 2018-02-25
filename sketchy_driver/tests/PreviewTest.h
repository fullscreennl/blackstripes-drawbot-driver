#include <assert.h>
#include "../Preview.h"
#include "../bool.h"

#ifndef PREVIEW_TEST_H
#define PREVIEW_TEST_H

void Preview_test(){

    Preview *pr = Preview_alloc(1000,1000,"tests/test_image.png",Config_maxDelay(),Config_minDelay());
    Preview_setPixel(pr,100,100,Config_maxDelay(), true);
    Preview_setPixel(pr,110,110,Config_minDelay(), true);
    int bandWidth = Config_maxDelay() - Config_minDelay();
    Preview_setPixel(pr, 120, 120, Config_minDelay()+bandWidth/2.0, true);
    Preview_save(pr);
    Preview_release(pr);

    //Maybe compare the generated file with a fixed test image and assert that.
    //For now inspect manually, we just need to assure that fast pixels
    //are red and slow pixles are green
    assert(1);

}

#endif
