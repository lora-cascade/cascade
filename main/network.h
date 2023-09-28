#ifndef NETWORK_H_
#define NETWORK_H_

#include <stdbool.h>
#include <stdint.h>

#define MAX_CONNECTIONS 256

typedef struct {
    uint8_t id;
    bool active;
} network_member;

static network_member network[MAX_CONNECTIONS];

bool is_member(uint8_t id);

void join_network(uint8_t id);

#endif  // NETWORK_H_
