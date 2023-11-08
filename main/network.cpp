#include "network.h"

bool is_member(uint8_t id) {
    return network[id].active;
}

void join_network(uint8_t id) {
    network[id].active = true;
}
