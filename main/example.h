#ifndef EXAMPLE_H_
#define EXAMPLE_H_

#include <esp_intr_alloc.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdint.h>
#include <stdlib.h>
#include "lora.h"
#include "packet.h"

const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";

void example() {
    while (1) {
        vTaskDelay((rand() % 1000) / portTICK_PERIOD_MS);
        if (has_message()) {
            packet_t* message = get_message();
            if (message == NULL) {
                printf("WHYYYYYYYYYYYYYY\n");
            }
            printf("Received message id %d: ", message->header.message_id);
            for (int i = 0; i < message->header.data_length; i++) {
                printf("%c", message->data[i]);
            }
            printf("\n");
            free(message);
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
        uint8_t data[10];
        for (int i = 0; i < 10; i++) {
            data[i] = charset[rand() % (sizeof(charset) - 1)];
        }
        int16_t id = send_message(data, 10);
        printf("Sent message id %d: %.10s\n", id, data);
    }
}

#endif  // EXAMPLE_H_
