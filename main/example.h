#ifndef EXAMPLE_H_
#define EXAMPLE_H_

#include <esp_intr_alloc.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdint.h>
#include <stdlib.h>
#include "lora_interface.h"
#include "packet.h"

const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
int counter = 0;

void example() {
    while (1) {
        vTaskDelay(10 / portTICK_PERIOD_MS);
        if (has_message()) {
            packet_t* message = get_message();
            if (message == NULL) {
                printf("WHYYYYYYYYYYYYYY\n");
                continue;
            }
            printf("Received message id %d: %.*s\n", message->header.message_id, message->header.data_length, message->data);
            char data[255];
            int length = sprintf(data, "%.11s %d", message->data, message->header.message_id);
            send_message((uint8_t*)data, length);
            free(message);
            counter = 0;
            continue;
        }
        if (counter > 1000) {
            printf("counter activated\n");
            send_message((uint8_t*)"hello world", 11);
            counter = 0;
            continue;
        }
        counter++;
    }
}

#endif  // EXAMPLE_H_
