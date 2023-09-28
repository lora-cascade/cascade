#ifndef PACKET_H_
#define PACKET_H_

#include <stdbool.h>
#include <stdint.h>
#include "command.h"

static uint8_t device_id = 0;
static bool changed_device_id = false;
static uint16_t last_message_id = 0;

typedef enum { NONE, LARGE_PACKET } error_type;

error_type error = NONE;

typedef struct {
    uint8_t sender_id;
    uint16_t message_id;
    uint8_t data_length;
    uint8_t command;
} header;

typedef struct {
    header header;
    char* data;
} packet;

uint8_t get_device_id();

void set_device_id(uint8_t id);

void set_error(error_type err);

error_type get_error();

packet create_packet(char* data, uint8_t data_length);

#endif  // PACKET_H_
