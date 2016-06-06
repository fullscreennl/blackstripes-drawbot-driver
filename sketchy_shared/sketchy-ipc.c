//gcc test_server.c sketchy-ipc.c -o server
//gcc test_driver.c sketchy-ipc.c -o driver
//list: ipcs -m
//remove: ipcrm -M 1234
//remove: ipcrm -M 4567

#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sketchy-ipc.h"

//driver state
static int driverstate_shmsize = sizeof(DriverState);
static DriverState *driverstate_shm_pt;
static int driverstate_shm_id;

//driver commands
static int command_shmsize = sizeof(DriverCommand);
static DriverCommand *command_shm_pt;
static int command_shm_id;

void updateDriverState(DriverSatusCode statusCode,const char *joburl,const char *name){
    DriverState *driver_state = driverState();
    driver_state->messageID ++;
    driver_state->statusCode = statusCode;
    strcpy( driver_state->joburl , joburl );
    strcpy( driver_state->name , name );
}

DriverState *driverState(){
    return driverstate_shm_pt;
}

void setCommand(char *msg, CommandCode command, float floatValue, int intValue){
    DriverCommand *cmd = getCommand();
    cmd->messageID ++;
    cmd->commandCode = command;
    strcpy(cmd->msg,msg);
    cmd->fvalue = floatValue;
    cmd->ivalue = intValue;
}

DriverCommand *getCommand(){
    return command_shm_pt;
}

void shmCreate()
{

    key_t shm_key = 1234;
    key_t shm_key2 = 4567;

    // Create our memory segments
    if((driverstate_shm_id = shmget(shm_key, driverstate_shmsize, IPC_CREAT | 0600)) < 0)
    {
        perror("shmget");
        exit(1);
    }

    if((command_shm_id = shmget(shm_key2, command_shmsize, IPC_CREAT | 0600)) < 0)
    {
        perror("shmget");
        exit(1);
    }

    // Attach memory segments
    if((driverstate_shm_pt = shmat(driverstate_shm_id, NULL, 0)) == (DriverState *)-1)
    {
        perror("shmat");
        exit(1);
    }

    if((command_shm_pt = shmat(command_shm_id, NULL, 0)) == (DriverCommand *)-1)
    {
        perror("shmat");
        exit(1);
    }

}

void shmDestroy(){

    // Detach the shared memory segments 
    if (shmdt(driverstate_shm_pt) == -1)
    {
        perror("shmdt");
        exit(1);
    }

    if (shmdt(command_shm_pt) == -1)
    {
        perror("shmdt");
        exit(1);
    }

    // Free and delete the memory segments
    if(shmctl(driverstate_shm_id, IPC_RMID, 0) < 0)
    {
        perror("shmctl");
        exit(1);
    }

    if(shmctl(command_shm_id, IPC_RMID, 0) < 0)
    {
        perror("shmctl");
        exit(1);
    }

}



