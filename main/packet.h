#ifndef PACKET_H_
#define PACKET_H_

#include <stdbool.h>
#include <stdint.h>
#include "command.h"

typedef enum { NONE, LARGE_PACKET } error_type;

typedef struct {
    uint8_t sender_id;
    uint16_t message_id;
    uint8_t data_length;
    uint8_t command;
} __attribute__((packed)) header_t;

typedef struct {
    header_t header;
    uint8_t data[];
} __attribute__((packed)) packet_t;

uint8_t get_device_id();

void set_device_id(uint8_t id);

void set_error(error_type err);

error_type get_error();

packet_t* create_packet(uint8_t* data, uint8_t data_length);

#endif  // PACKET_H_
