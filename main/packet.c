#include "packet.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static uint8_t device_id = 0;
static bool changed_device_id = false;
static uint16_t last_message_id = 0;

static error_type error = NONE;

uint8_t get_device_id() {
    return device_id;
}

void set_device_id(uint8_t id) {
    changed_device_id = true;
    device_id = id;
}

void set_error(error_type err) {
    error = err;
}

error_type get_error() {
    return error;
}

packet_t* create_packet(uint8_t* data, uint8_t data_length) {
    header_t header = {
        .sender_id = device_id,
        .message_id = last_message_id++,
        .data_length = data_length,
    };

    if (data_length > 255 - sizeof(header_t)) {
        set_error(LARGE_PACKET);
        return NULL;
    }

    packet_t* packet = malloc(sizeof(header_t) + data_length);
    packet->header = header;
    memcpy(packet->data, data, data_length);

    return packet;
}
