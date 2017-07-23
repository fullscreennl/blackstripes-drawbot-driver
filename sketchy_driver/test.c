#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "Model.h"
#include "Config.h"

#include "tests/SpeedManagerTest.h"
#include "tests/PreviewTest.h"
#include "tests/PointTest.h"

#include "sketchy-ipc.h"

int main(int argc, char *argv[]){

    Config_setBasePath("../build/job/");
    Config_load("../build/job/manifest.ini"); //test code assumes 1000px x 1000px

    shmCreate();

    Model_createInstance();

    SpeedManager_test();
    printf("SpeedManager test PASSED\n");

    Preview_test();
    printf("Preview test PASSED\n");

    Point_test();
    printf("Point test PASSED\n");

    return 0;
}

