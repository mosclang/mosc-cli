//
// Created by Mahamadou DOUMBIA [OML DSI] on 22/02/2022.
//
#include "cli.h"
#include <string.h>
#include "runtime.h"

void setRootDirectory(MVM* vm) {
    const char* dir = MSCGetSlotString(vm,1);
    char* copydir = malloc(strlen(dir)+1);
    strcpy(copydir, dir);
    rootDirectory = copydir;
}