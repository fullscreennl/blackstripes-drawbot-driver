#ifndef SKETCHY_IPC_H
#define SKETCHY_IPC_H

typedef enum {
    driverSatusCodeBusy = 1,
    driverSatusCodeIdle = 2,
    driverStatusCodePaused = 3,
    driverStateOutOfBoundsError = 4,
    driverStateNoDataFoundInSVGError = 5,
}DriverSatusCode;

typedef struct DriverState{
    int messageID;
    DriverSatusCode statusCode;
    char name[100];
    char joburl[100];
}DriverState;

typedef enum {
    commandCodeNone = 0,
    commandCodeStop = 1,
    commandCodeSpeed = 2,
    commandCodePenMode = 3,
    commandCodePause = 4,
    commandCodeResume = 5,
    commandCodePreviewAbort = 6,
}CommandCode;

typedef struct DriverCommand{
    int messageID;
    CommandCode commandCode;
    char msg[100];
    float fvalue;
    int ivalue;
}DriverCommand;

void shmCreate();
void shmDestroy();

void updateDriverState(DriverSatusCode statusCode,const char *joburl,const char *name);
DriverState *driverState();

void setCommand(char *msg, CommandCode command, float floatValue, int intValue);
DriverCommand *getCommand();

#endif
