#ifndef CHUE_APPLICATION_ROOMLIST_H
#define CHUE_APPLICATION_ROOMLIST_H

#include <stdio.h>

#include <domain/room.h>

typedef struct RoomList {
    Room *items;
    size_t count;
} RoomList;

#endif // CHUE_APPLICATION_ROOMLIST_H
