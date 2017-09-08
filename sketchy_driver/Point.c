#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "Point.h"
#include "FSObject.h"
#include "FSArray.h"
#include "FSNumber.h"
#include "machine-settings.h"
#include "Model.h"

void Point_calculateAngle(Point *p);

float Point_toRadians(float deg){
    return (float)deg * M_PI/180.0;
}

float Point_trimAngle(float inputAngle){
    int deg = (int)round(inputAngle);
    int degTrimmed = (int)round(inputAngle)%360;
    float toremove = deg - degTrimmed;
    return inputAngle - toremove;
}

float Point_needsPositionUpdateWith(float x, float y){
    if(x > BOT->currentCenter + BOT_REPOSITION_THRESHOLD){
        return x;
        // return x - BOT_REPOSITION_THRESHOLD;
    }else if(x < BOT->currentCenter - BOT_REPOSITION_THRESHOLD){
        // return x + BOT_REPOSITION_THRESHOLD;
        return x;
    }
    return BOT->currentCenter;
}

FSArray *Point_findCircleCircleIntersections(float cx0, float cy0,float radius0,float cx1,float cy1,float radius1){

    //based on: http://csharphelper.com/blog/2014/09/determine-where-two-circles-intersect-in-c/

    //Find the distance between the centers.
    float dx = cx0 - cx1;
    float dy = cy0 - cy1;
    float dist = sqrt(dx * dx + dy * dy);

    //See how manhym solutions there are.
    if (dist > radius0 + radius1){
        //No solutions, the circles are too far apart.
        return NULL;
    }else if(dist < abs(radius0 - radius1)){
        //No solutions, one circle contains the other.
        return NULL;
    }else if ((dist == 0) && (radius0 == radius1)){
        //No solutions, the circles coincide.
        return NULL;
    }else{

        // Find a and h.
        float a = (radius0 * radius0 - radius1 * radius1 + dist * dist) / (2 * dist);
        float h = sqrt(radius0 * radius0 - a * a);

        // Find P2.
        float cx2 = cx0 + a * (cx1 - cx0) / dist;
        float cy2 = cy0 + a * (cy1 - cy0) / dist;

        // Get the points P3.
        float x1 = (cx2 + h * (cy1 - cy0) / dist);
        float y1 = (cy2 - h * (cx1 - cx0) / dist);
        Point * intersection1 = Point_alloc(x1,y1);

        float x2 = (cx2 - h * (cy1 - cy0) / dist);
        float y2 = (cy2 + h * (cx1 - cx0) / dist);
        Point * intersection2 = Point_alloc(x2,y2);

        FSArray *arr = FSArray_alloc(2);

        // See if we have 1 or 2 solutions.
        if (dist == radius0 + radius1){ 
            FSArray_append(arr,intersection1);
            FSArray_append(arr,intersection1);
            Point_release(intersection1);
            Point_release(intersection2);
            return arr;
        }else{
            FSArray_append(arr,intersection1);
            FSArray_append(arr,intersection2);
            Point_release(intersection1);
            Point_release(intersection2);
            return arr;
        }

    }

}

Point *Point_alloc(float x, float y){
    Point *p = (Point *) malloc(sizeof(Point));
    p->x = x;
    p->y = y;
    p->retainCount = 1;
    p->type = "Point";
    return p;
}

void Point_copy(Point *src,Point *dest){
    dest->x = src->x;
    dest->y = src->y;
    dest->left_angle = src->left_angle;
    dest->right_angle = src->right_angle;
    dest->left_steps = src->left_steps;
    dest->right_steps = src->right_steps;
}

void Point_log(Point *p){
    printf("Point <%p left:%i l-angle:%f right:%i r-angle:%f - x:%f  y:%f >\n",
            p,p->left_steps,p->left_angle,p->right_steps,p->right_angle,p->x,p->y);
}

void Point_release(Point *p){
    FSObject_release(p);
}

void Point_retain(Point *p){
    FSObject_retain(p);
}

void Point_setNull(Point *p){
    p->x = 0.0;
    p->y = 0.0;
}


Point * Point_xyFromEngineStates(float leftAng, float rightAng){

    float xnull = Model_getCenter();
    float ynull = SHOULDER_HEIGHT+CANVAS_Y;

    float left = leftAng - 180.0 - 180.0;
    float right = -rightAng + 180.0;

    // left = round(left / ANGLE_PER_STEP) * ANGLE_PER_STEP;
    // right = round(right / ANGLE_PER_STEP) * ANGLE_PER_STEP;

    float lx = cos(Point_toRadians(left)) * UPPER_ARM_LENGTH + Model_getLeftShoulderX();
    float ly = sin(Point_toRadians(left)) * UPPER_ARM_LENGTH + LEFT_SHOULDER_POS_Y;

    float rx = cos(Point_toRadians(right)) * UPPER_ARM_LENGTH + Model_getRightShoulderX();
    float ry = sin(Point_toRadians(right)) * UPPER_ARM_LENGTH + RIGHT_SHOULDER_POS_Y;

    ly = ly - ynull;
    ry = ry - ynull;
    lx = lx - xnull;
    rx = rx - xnull;

    FSArray *joint = Point_findCircleCircleIntersections(lx,ly,LOWER_ARM_LENGTH,rx,ry,LOWER_ARM_LENGTH);
    Point *point1 = FSArray_objectAtIndex(joint,0);
    Point *point2 = FSArray_objectAtIndex(joint,1);
    FSArray_release(joint);

    float angle_in_rad_left;

    if(point1->y > point2->y){
        angle_in_rad_left = atan2(point1->y - ly, point1->x - lx);
    }else{
        angle_in_rad_left = atan2(point2->y - ly, point2->x - lx);
    }

    float mx = cos(angle_in_rad_left) * (LOWER_ARM_LENGTH + EXENSION1) + lx;
    float my = sin(angle_in_rad_left) * (LOWER_ARM_LENGTH + EXENSION1) + ly;

    return Point_alloc(mx + Model_getCenter(), my);

}

FSArray * Point_engineStatesFromXYonCanvas(float _x, float _y){

    float xnull = Model_getCenter();
    float ynull = SHOULDER_HEIGHT+CANVAS_Y;

    float x = xnull + _x - Model_getCenter();
    float y = ynull + _y;

    float lsx = Model_getLeftShoulderX();
    float lsy = LEFT_SHOULDER_POS_Y;
    float rsx  = Model_getRightShoulderX();
    float rsy = RIGHT_SHOULDER_POS_Y;

    Point *leftelbow;
    Point *rightelbow;

    FSArray *leftpoints = Point_findCircleCircleIntersections(x,y,LOWER_ARM_LENGTH + EXENSION1,lsx,lsy,UPPER_ARM_LENGTH);
    Point *leftpoint_1 = FSArray_objectAtIndex(leftpoints,0);
    Point *leftpoint_2 = FSArray_objectAtIndex(leftpoints,1);
    if (leftpoint_1->x < leftpoint_2->x){
        leftelbow = leftpoint_1;
    }else{
        leftelbow = leftpoint_2;
    }

    float angle_in_degrees_left = atan2(leftelbow->y - lsy, leftelbow->x - lsx) * 180.0 / M_PI;
    float angle_in_rad_left = atan2(y - leftelbow->y, x - leftelbow->x);

    float mx = cos(angle_in_rad_left) * (LOWER_ARM_LENGTH) + leftelbow->x;
    float my = sin(angle_in_rad_left) * (LOWER_ARM_LENGTH) + leftelbow->y;

    FSArray *rightpoints = Point_findCircleCircleIntersections(mx,my,LOWER_ARM_LENGTH,rsx,rsy,UPPER_ARM_LENGTH);
    Point *rightpoint_1 = FSArray_objectAtIndex(rightpoints,0);
    Point *rightpoint_2 = FSArray_objectAtIndex(rightpoints,1);
    if (rightpoint_1->x > rightpoint_2->x){
        rightelbow = rightpoint_1;
    }else{
        rightelbow = rightpoint_2;
    }

    float angle_in_degrees_right = atan2(rightelbow->y - rsy, rightelbow->x - rsx) * 180.0 / M_PI;

    angle_in_degrees_left = angle_in_degrees_left + 180.0 + 180.0 ;
    angle_in_degrees_left = Point_trimAngle(angle_in_degrees_left);
    angle_in_degrees_right = -angle_in_degrees_right + 180.0;
    angle_in_degrees_right = Point_trimAngle(angle_in_degrees_right);

    FSArray *arr = FSArray_alloc(2);
    FSNumber *angle_in_degrees_left_num = FSNumber_allocWithFloat(angle_in_degrees_left);
    FSNumber *angle_in_degrees_right_num = FSNumber_allocWithFloat(angle_in_degrees_right);
    FSArray_append(arr,angle_in_degrees_left_num);
    FSArray_append(arr,angle_in_degrees_right_num);

    FSNumber_release(angle_in_degrees_left_num);
    FSNumber_release(angle_in_degrees_right_num);

    FSArray_release(leftpoints);
    FSArray_release(rightpoints);

    return arr;

}

Point *Point_allocWithXY(float x, float y){
    Point *p = Point_alloc(x, y);
    Point_calculateAngle(p);
    return p;
}

void Point_updateWithXY(Point *p,float x, float y){


    p->x = x;
    p->y = y;
    Point_calculateAngle(p);
}

Point *Point_allocWithSteps(int left_steps, int right_steps){
    Point *p = (Point *) malloc(sizeof(Point));
    p->left_angle = 180.0 + left_steps * ANGLE_PER_STEP;
    p->right_angle = 180.0 + right_steps * ANGLE_PER_STEP;
    p->left_steps = left_steps;
    p->right_steps = right_steps;
    p->retainCount = 1;
    p->type = "Point";
    Point *p2 = Point_xyFromEngineStates(p->left_angle,p->right_angle);
    p->x = p2->x;
    p->y = p2->y;
    Point_release(p2);
    return p;
}

void Point_updateWithSteps(Point *p,int left_steps, int right_steps){
    p->left_angle = 180.0 + left_steps * ANGLE_PER_STEP;
    p->right_angle = 180.0 + right_steps * ANGLE_PER_STEP;
    p->left_steps = left_steps;
    p->right_steps = right_steps;
    Point *p2 = Point_xyFromEngineStates(p->left_angle,p->right_angle);
    p->x = p2->x;
    p->y = p2->y;
    Point_release(p2);
}

void Point_calculateAngle(Point *p){

    FSArray *angles  = Point_engineStatesFromXYonCanvas(p->x,p->y);
    FSNumber *angle_1 = (FSNumber*)FSArray_objectAtIndex(angles,0);
    FSNumber *angle_2 = (FSNumber*)FSArray_objectAtIndex(angles,1);
    //printf("angle left: %f \n\n", angle_1->floatValue);
    p->left_angle = angle_1->floatValue;
    p->right_angle = angle_2->floatValue;
    //float leftAngle = p->left_angle;
    if(p->left_angle < 0 ){
        //p->left_angle =  360.0 - p->left_angle;
    }
    p->left_steps = (int)round((p->left_angle - 180.0) / ANGLE_PER_STEP);
    //printf("steps left from angle: %i %f\n\n", p->left_steps, p->left_angle );
    p->right_steps = (int)round((p->right_angle - 180.0) / ANGLE_PER_STEP);

    FSArray_release(angles);
}

