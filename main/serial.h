#ifndef SERIAL_H_
#define SERIAL_H_

#include <Arduino.h>
#include <stdint.h>
#include <vector>
#include "lora_interface.h"

void example() {
    Serial.begin(115200);
    Serial.write('\n');
    std::vector<uint8_t> input;
    while(1) {
        vTaskDelay(1);
        if(Serial.available() > 0) {
            auto incoming = Serial.read();
            Serial.write(incoming);
            if(incoming != 13) {
                input.push_back(incoming);
            } else {
                Serial.write('\n');
                send_message(input.data(), input.size());
                input.clear();
            }
        }

        if(has_message()) {
            auto* message = get_message();
            /* printf("Received message %.*s\n", message->header.data_length, message->data); */
            Serial.write("Received message ");
            for(int i = 0; i < message->header.data_length; i++) {
                Serial.write(message->data[i]);
            }
            Serial.write('\n');
            free(message);
        }
    }
}

#endif // SERIAL_H_
