#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "Config.h"

#include "tests/InputImageTest.h"
#include "tests/SpeedManagerTest.h"
#include "tests/PreviewTest.h"
#include "tests/PointTest.h"

#include "sketchy-ipc.h"

int main(int argc, char *argv[]){

    Config_setBasePath("../test_assets/");
    Config_load("../test_assets/test.ini"); //test code assumes 1000px x 1000px

    shmCreate();
    
    InputImage_testSaveAsPNG();
    InputImage_testBestPointOfNDestinationsFromXY();
    InputImage_testClearDarkness();
    InputImage_testLineDarkness();
    InputImage_test();
    InputImage_testGetBrightness();
    printf("\n\nInputImage test PASSED\n");
    
    SpeedManager_test();
    printf("SpeedManager test PASSED\n");

    Preview_test();
    printf("Preview test PASSED\n");

    Point_test();
    printf("Point test PASSED\n");
    
    return 0;
}

