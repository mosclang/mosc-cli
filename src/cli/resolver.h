//
// Created by Mahamadou DOUMBIA [OML DSI] on 21/02/2022.
//

#ifndef MOSC_RESOLVER_H
#define MOSC_RESOLVER_H

#include "msc.h"

extern MVM *resolver;
void initResolverVM();
char* MOSCResolveModule(const char* importer, const char* module);
char* MOSCLoadModule(const char* module);
void freeResolver();

#endif //MOSC_RESOLVER_H
