#include <stdlib.h>

#include <domain/room.h>

void room_free(Room *r) {
    free(r->name);
}
