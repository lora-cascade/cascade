#ifndef LORA_H_
#define LORA_H_

#include <driver/spi_common.h>
#include <driver/spi_master.h>
#include <esp_log.h>
#include <stddef.h>
#include <sx127x.h>

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

sx127x *device = NULL;

int32_t init_lora();

int16_t send_message(char* message);

#endif // LORA_H_
