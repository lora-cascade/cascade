#ifndef LORA_H_
#define LORA_H_

#include <driver/gpio.h>
#include <driver/spi_common.h>
#include <driver/spi_master.h>
#include <esp_log.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "data.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "freertos/projdefs.h"
#include "hal/spi_types.h"
#include "packet.h"
#include <lora.h>
#include <sys/time.h>
#include <esp_random.h>

#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 23
#define DIO0 26

#define SYNCWORD 37
#define SPREADING_FACTOR 7
#define BANDWIDTH 7
#define CODING_RATE 1

#define MAX_QUEUE_SIZE 100

#define CSMA_SLOT_TIME_US (int64_t)20

int get_size();

int32_t init_lora();

int16_t send_message(uint8_t* message, uint8_t data_length);

packet_t* get_message();

bool has_message();

#endif  // LORA_H_
