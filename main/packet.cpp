#include "packet.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <cstddef>
#include <set>
#include "command.h"

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
        .command = SEND_MESSAGE,
    };

    if (data_length > 255 - sizeof(header_t)) {
        set_error(LARGE_PACKET);
        return NULL;
    }

    packet_t* packet = (packet_t*)malloc(sizeof(header_t) + data_length);
    packet->header = header;
    memcpy(packet->data, data, data_length);

    return packet;
}

packet_t* create_ack() {
    header_t header = {
        .sender_id = device_id,
        .message_id = 0,
        .data_length = 0,
        .command = ACK,
    };

    packet_t* packet = (packet_t*)malloc(sizeof(header_t));
    packet->header = header;

    return packet;
}

packet_t* create_join() {
    header_t header = {
        .sender_id = device_id,
        .message_id = 0,
        .data_length = 0,
        .command = JOIN_NETWORK,
    };

    packet_t* packet = (packet_t*)malloc(sizeof(header_t));
    packet->header = header;

    return packet;
}

packet_t* create_join_return(std::set<uint8_t>& known_ids) {
    header_t header = {
        .sender_id = device_id,
        .message_id = 0,
        .data_length = (uint8_t)known_ids.size(),
        .command = ACK_NETWORK,
    };

    packet_t* packet = (packet_t*)malloc(sizeof(header_t) + known_ids.size());
    packet->header = header;
    int i = 0;
    for(auto id : known_ids) {
        packet->data[i] = id;
        i++;
    }

    return packet;
}
