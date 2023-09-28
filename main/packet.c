#include "packet.h"
#include <stdint.h>

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

packet create_packet(char* data, uint8_t data_length) {
    header header = {
        .sender_id = device_id,
        .message_id = last_message_id++,
        .data_length = data_length,
    };

    packet packet = {
        .header = header,
        .data = data,
    };

    if (data_length > 255 - sizeof(header)) {
        set_error(LARGE_PACKET);
    }

    return packet;
}
