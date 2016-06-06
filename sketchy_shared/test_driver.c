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
        DriverCommand *cmd = getCommand();
        if(cmd->messageID != lastReadMessageID){
            printf("Readed from SHM cmd ID: %i\n", cmd->messageID);
            printf("Readed from SHM cmd code: %i\n", cmd->commandCode);
            printf("Readed from SHM cmd msg: %s\n", cmd->msg);
            lastReadMessageID = cmd->messageID;
        }
        updateDriverState(argv[1]);
        sleep(2);
    }
    return 0;
}
