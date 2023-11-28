#ifndef RECEIVER_H_
#define RECEIVER_H_

#include "lora_interface.h"
#include "packet.h"

void receiver() {
    while(1) {
        vTaskDelay(1);
        if(has_message()) {
            packet_t* message = get_message();
            printf("Received message id %d: %.*s\n", message->header.message_id, message->header.data_length, message->data);
            free(message);
        }
    }
}

#endif // RECEIVER_H_
