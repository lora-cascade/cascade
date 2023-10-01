#ifndef LORA_H_
#define LORA_H_

#include <driver/spi_common.h>
#include <driver/spi_master.h>
#include <esp_log.h>
#include <stddef.h>
#include <stdint.h>
#include <sx127x.h>
#include "common.h"
#include "data.h"
#include "packet.h"

#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 23
#define DIO0 26

#define FREQUENCY 910200012
#define SYNCWORD 37

#define SPREADING_FACTOR SX127x_SF_9
#define BANDWIDTH SX127x_BW_125000

#define MAX_QUEUE_SIZE 10

static sx127x* device = NULL;

static TaskHandle_t handle_interrupt;

static QueueHandle_t receive_queue;

static QueueHandle_t send_queue;

int32_t init_lora();

int16_t send_message(uint8_t* message, uint8_t data_length);

packet_t* get_message();

bool has_message();

static void pop_queue(QueueHandle_t* handle);

static data_t get_next_message();

static bool has_next_message();

static void add_message(packet_t* packet);

static void tx_callback(sx127x* device);

static void rx_callback(sx127x* device, uint8_t* data, uint16_t data_length);

static void cad_callback(sx127x* device, int cad_detected);

static void task_handler(void* arg);

#endif  // LORA_H_
