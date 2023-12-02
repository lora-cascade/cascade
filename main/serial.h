#ifndef SERIAL_H_
#define SERIAL_H_

#include <Arduino.h>
#include <stdint.h>
#include <vector>
#include "command.h"
#include "freertos/projdefs.h"
#include "lora_interface.h"
#include "packet.h"

bool listen_for_kill_state = false;

void handle_send_kill_message() {
    uint8_t status = Serial.read();
    if(status != 0) {
        status = 1;
    }
    set_kill_status(status);
    packet_t kill_packet = create_kill_packet(status);
    send_packet(&kill_packet);
    Serial.write(0x01);
}

void handle_listen_state() {
    if (Serial.available() > 0) {
        uint8_t in = Serial.read();
        if (in == 0x05) {
            Serial.write(0x02);
            listen_for_kill_state = false;
            return;
        }
    }
    bool kill_status = get_kill_status();
    Serial.write((uint8_t)kill_status);
    vTaskDelay(pdMS_TO_TICKS(250));
}

void handle_list_devices() {
    uint8_t list[255];
    uint8_t count = get_devices(list);
    Serial.write(count);
    for (int i = 0; i < count; i++) {
        Serial.write(list[i]);
    }
}

void handle_poll_messages() {
    uint8_t count = get_message_count();
    Serial.write(count);
    for (int i = 0; i < count; i++) {
        packet_t packet = get_message();
        Serial.write(packet.header.data_length);
        int offset = 0;
        if(packet.header.command == DIRECTED_MESSAGE) {
            offset = 1;
        }
        for (int j = 0; j < packet.header.data_length; j++) {
            Serial.write(packet.data[j+1]);
        }
    }
}

void handle_send_message() {
    uint8_t receiver = Serial.read();
    uint8_t length = Serial.read();
    uint8_t message[255];
    for(int i = 0; i < length; i ++) {
        message[i] = Serial.read();
    }
    send_directed_message(message, length, receiver);
    Serial.write(0x01);
}

void handle_serial() {
    Serial.begin(115200);
    while (1) {
        vTaskDelay(1);
        if (listen_for_kill_state)
            handle_listen_state();
        else if (Serial.available() > 0) {
            uint8_t incoming = Serial.read();
            switch (incoming) {
                case 0x00:
                    handle_send_message();
                    break;
                case 0x01:
                    handle_poll_messages();
                    break;
                case 0x02:
                    handle_list_devices();
                    break;
                case 0x03:
                    handle_send_kill_message();
                    break;
                case 0x04:
                    listen_for_kill_state = true;
                    break;
                case 0x05:
                    Serial.write(0x02);
                    break;
            }
            /* Serial.write(incoming); */
        }
    }
}

#endif  // SERIAL_H_
