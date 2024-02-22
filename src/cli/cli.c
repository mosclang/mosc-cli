//
// Created by Mahamadou DOUMBIA [OML DSI] on 22/02/2022.
//
#include "cli.h"
#include <string.h>
#include "runtime.h"

void setRootDirectory(Djuru* djuru) {
    const char* dir = MSCGetSlotString(djuru,1);
    char* copydir = malloc(strlen(dir)+1);
    strcpy(copydir, dir);
    rootDirectory = copydir;
}