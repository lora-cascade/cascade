#ifndef PACKET_H_
#define PACKET_H_

#include <stdbool.h>
#include <stdint.h>

static uint8_t device_id = 0;
static bool changed_device_id = false;
static uint16_t last_message_id = 0;

static enum error_type {
    NONE, LARGE_PACKET
} error = NONE;

struct header {
    uint8_t sender_id;
    uint16_t message_id;
    uint8_t data_length;
};

struct packet {
    struct header header;
    char* data;
};

uint8_t get_device_id();

void set_device_id(uint8_t id);

void set_error(enum error_type err);

enum error_type get_error();

struct packet create_packet(char* data, uint8_t data_length);

#endif  // PACKET_H_
