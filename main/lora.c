#include "lora.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "hal/spi_types.h"
#include "sx127x.h"

static void tx_callback(sx127x* device) {
    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_CAD, SX127x_MODULATION_LORA, device));
}

static void rx_callback(sx127x* device, uint8_t* data, uint16_t data_length) {}

int32_t init_lora() {
    spi_bus_config_t config = {
        .mosi_io_num = MOSI,
        .miso_io_num = MISO,
        .sclk_io_num = SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &config, 1));

    spi_device_interface_config_t dev_cfg = {.clock_speed_hz = 3E6,
                                             .spics_io_num = SS,
                                             .queue_size = 16,
                                             .command_bits = 0,
                                             .address_bits = 8,
                                             .dummy_bits = 0,
                                             .mode = 0};

    spi_device_handle_t spi_device;
    ESP_ERROR_CHECK(spi_bus_add_device(HSPI_HOST, &dev_cfg, &spi_device));
    ESP_ERROR_CHECK(sx127x_create(spi_device, &device));
    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_SLEEP, SX127x_MODULATION_LORA, device));
    ESP_ERROR_CHECK(sx127x_set_frequency(FREQUENCY, device));
    ESP_ERROR_CHECK(sx127x_lora_reset_fifo(device));
    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_STANDBY, SX127x_MODULATION_LORA, device));
    ESP_ERROR_CHECK(sx127x_lora_set_bandwidth(BANDWIDTH, device));
    ESP_ERROR_CHECK(sx127x_lora_set_implicit_header(NULL, device));
    ESP_ERROR_CHECK(sx127x_lora_set_modem_config_2(SPREADING_FACTOR, device));
    ESP_ERROR_CHECK(sx127x_lora_set_syncword(SYNCWORD, device));
    ESP_ERROR_CHECK(sx127x_set_preamble_length(8, device));
    sx127x_tx_set_callback(tx_callback, device);

    return 0;
}

int16_t send_message(char* message) {
    return 0;
}
