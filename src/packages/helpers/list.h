#ifndef MOSCC_LIST_H
#define MOSCC_LIST_H

#include <stdlib.h>

typedef struct {
    void **data;
    int capacity;
    int size;

} MSCList;

// From: http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2Float
int powerOf2Ceiled(int n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;

    return n;
}

MSCList *newList(int capacity) {
    MSCList *l = (MSCList *) malloc(sizeof(List));
    l->data = (void **) malloc(sizeof(*l->data) * capacity);
    l->capacity = capacity;
    l->size = 0;
    return l;
}

void *getFromList(MSCList *list, int index) {
    if (index >= list->size) {
        return NULL;
    }
    return list->data[index];
}

void setInList(MSCList *list, int index, void *value) {
    if (index >= list->size) {
        return;
    }
    list->data[index] = value;
}

void pushInList(MSCList *list, void *value) {
    if (list->size + 1 >= list->capacity) {
        // resize
        int needed = powerOf2Ceiled(list->capacity + 1);
        list->data = (void **) realloc(list->data, sizeof(*list->data) * needed);
        list->capacity = needed;
        if (list->data == NULL) {
            return;
        }
    }
    list->data[list->size++] = value;
}

void popFromList(MSCList *list) {
    if (list->size <= 0) {
        return;
    }
    void *item = list->data[list->size--];
    // list->destroy(item);
}

void removeFromList(MSCList *list, int index) {
    if (index >= list->size || index < 0) {
        return;
    }
    void *item = list->data[index];
    for (int i = index; ; i++) {
        if(i == list->size - 1) {
            list->data[i] = NULL;
            break;
        }
        list->data[i] = list->data[i + 1];
    }
    // list->destroy(item);
}

#endif //MOSCC_LIST_H