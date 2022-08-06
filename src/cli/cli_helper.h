//
// Created by Mahamadou DOUMBIA [OML DSI] on 23/02/2022.
//

#ifndef MOSCC_CLI_HELPER_H
#define MOSCC_CLI_HELPER_H
#include <stdlib.h>
#include <string.h>

char* cli_strdup(const char* s) {
    size_t len = strlen(s) + 1;
    char* m = (char*)malloc(len);
    if (m == NULL) return NULL;
    return memcpy(m, s, len);
}

inline char* cli_strndup(const char* s, size_t n) {
    char* m;
    size_t len = strlen(s);
    if (n < len) len = n;
    m = (char*)malloc(len + 1);
    if (m == NULL) return NULL;
    m[len] = '\0';
    return memcpy(m, s, len);
}
#endif //MOSCC_CLI_HELPER_H
