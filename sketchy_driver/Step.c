//
//  Step.c
//  sketchy
//
//  Created by Johan Ten Broeke on 16/11/14.
//  Copyright (c) 2014 Johan Ten Broeke. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include "Step.h"
#include "FSObject.h"

Step *Step_alloc(StepperMotorDir leftengine, StepperMotorDir rightengine){
    Step *obj = (Step *) malloc(sizeof(Step));
    obj->leftengine = leftengine;
    obj->rightengine = rightengine;
    obj->retainCount = 1;
    obj->type = "Step";
    return obj;
}

Step *Step_update(Step *obj,StepperMotorDir leftengine, StepperMotorDir rightengine){
    obj->leftengine = leftengine;
    obj->rightengine = rightengine;
    return obj;
}


void Step_release(Step *obj){
    FSObject_release(obj);
}

void Step_retain(Step *obj){
    FSObject_retain(obj);
}
