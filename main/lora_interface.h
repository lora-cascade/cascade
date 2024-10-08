#ifndef LORA_H_
#define LORA_H_

#include <LoRa.h>
#include <driver/gpio.h>
#include <driver/spi_common.h>
#include <driver/spi_master.h>
#include <esp_log.h>
#include <esp_random.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "common.h"
#include "data.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "freertos/projdefs.h"
#include "hal/spi_types.h"
#include "packet.h"

#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 23
#define DIO0 26

#define FREQUENCY 915E6
#define SYNCWORD 37
#define SPREADING_FACTOR 7
#define BANDWIDTH 7
#define CODING_RATE 1

#define MAX_QUEUE_SIZE 100

#define CSMA_SLOT_TIME_MS (int64_t)20

int get_size();

int32_t init_lora();

int16_t send_message(uint8_t* message, uint8_t data_length);

int16_t send_directed_message(uint8_t* message, uint8_t data_length, uint8_t target);

void send_packet(packet_t* packet);

packet_t get_message();

uint8_t get_message_count();

bool has_message();

void set_kill_status(uint8_t status);

bool get_kill_status();

uint8_t get_devices(uint8_t* list);

#endif  // LORA_H_
