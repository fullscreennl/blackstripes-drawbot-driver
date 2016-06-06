#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sketchy-ipc.h"

static int lastReadMessageID;

int main(int argc, char *argv[])
{
    create();
    while(1){
        DriverState *state = driverState();
        if(state->messageID != lastReadMessageID){
            printf("Readed from SHM message ID: %i\n", state->messageID);
            printf("Readed from SHM status code: %i\n", state->statusCode);
            printf("Readed from SHM name: %s\n", state->name);
            printf("Readed from SHM joburl: %s\n", state->joburl);
            lastReadMessageID = state->messageID;
        }
        setCommand(argv[1]);
        sleep(2);
    }
    return 0;
}
