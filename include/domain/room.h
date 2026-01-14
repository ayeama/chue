#ifndef CHUE_DOMAIN_ROOM_H
#define CHUE_DOMAIN_ROOM_H

typedef struct Room {
    char id[37];
    char *name;
} Room;

void room_free(Room *r);

#endif  // CHUE_DOMAIN_ROOM_H
