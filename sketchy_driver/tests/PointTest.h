#include <math.h>
#include <assert.h>
#include "../Point.h"

#ifndef POINT_TEST_H
#define POINT_TEST_H

static void Point_testForXY(float x, float y){

    Point *p1 = Point_allocWithXY(x,y);
    Point_log(p1);

    Point *p2 = Point_allocWithSteps(p1->left_steps,p1->right_steps);
    Point_log(p2);

    //printf("- - - - - - - - - - - - - - - - - - - - - - -\n\n");
    // assert(p1->left_steps == p2->left_steps);
    // assert(p1->right_steps == p2->right_steps);

    // //ignore small rounding errors
    // assert(fabs(p1->x - p2->x) < 1.0);
    // assert(fabs(p1->y - p2->y) < 1.0);

    #ifndef __VPLOTTER__
    assert(fabs(p1->left_angle - p2->left_angle) < 1.0);
    assert(fabs(p1->right_angle - p2->right_angle) < 1.0);
    #endif

    Point_release(p1);
    Point_release(p2);

}

void Point_test(){

    #ifdef __VPLOTTER__

        Point *home = Point_allocWithSteps(0 ,0);
        Point_log(home);
        printf("-----\n");

        Point_testForXY(home->x,home->y);
        Point_testForXY(1000,1000);
        Point_testForXY(1500,1500);
        Point_testForXY(1457.500000,609.994873);

        Point_release(home);

    #else

        Point *home = Point_allocWithSteps(0 ,0);

        Point_testForXY(home->x,home->y);
        Point_testForXY(500,500);
        Point_testForXY(30,600);
        Point_testForXY(30.5,20.556);

        Point_release(home);

    #endif

}

#endif
