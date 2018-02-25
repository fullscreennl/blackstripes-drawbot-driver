//
//  Step.h
//  sketchy
//
//  Created by Johan Ten Broeke on 16/11/14.
//  Copyright (c) 2014 Johan Ten Broeke. All rights reserved.
//

#ifndef STEP_H
#define STEP_H

typedef enum stepperMotorDir{

    stepperMotorDirUp = 0,
    stepperMotorDirNone = 1,
    stepperMotorDirDown = 2

}StepperMotorDir;

typedef enum horizontalMovementDir{

    horizontalMovementDirLeft = 0,
    horizontalMovementDirNone = 1,
    horizontalMovementDirRight = 2

}HorizontalMovementDir;

typedef struct Step{
    int retainCount;
    char *type;
    StepperMotorDir leftengine;
    StepperMotorDir rightengine;
    HorizontalMovementDir horengine;
}Step;

Step *Step_alloc(StepperMotorDir leftengine, StepperMotorDir rightengine, HorizontalMovementDir horengine);
Step *Step_update(Step *obj,StepperMotorDir leftengine, StepperMotorDir rightengine, HorizontalMovementDir horengine);
void Step_release(Step *obj);
void Step_retain(Step *obj);


#endif
