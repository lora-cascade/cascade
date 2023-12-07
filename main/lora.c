#include "lora.h"
#include <stdint.h>
#include "freertos/projdefs.h"
#include "packet.h"
#include "sx127x.h"

static sx127x* device = NULL;

static TaskHandle_t handle_interrupt;

static QueueHandle_t receive_queue;

static QueueHandle_t send_queue;

static void pop_queue(QueueHandle_t* handle);

static data_t get_next_message();

static bool has_next_message();

static void add_message(packet_t* packet);

static void tx_callback(sx127x* device);

static void rx_callback(sx127x* device, uint8_t* data, uint16_t data_length);

static void cad_callback(sx127x* device, int cad_detected);

static void task_handler(void* arg);

bool waited = false;

static void tx_callback(sx127x* device) {
    if (has_next_message()) {
        data_t data = get_next_message();
        uint16_t id = *(uint16_t*)&data.data[1];
        /* printf("Sent %d\n", id); */
        if (data.data != NULL) {
            ESP_ERROR_CHECK(sx127x_lora_tx_set_for_transmission(data.data, data.size, device));
            free(data.data);
        }
    }

    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_TX, SX127x_MODULATION_LORA, device));
}

void IRAM_ATTR handle_interrupt_fromisr(void* arg) {
    xTaskResumeFromISR(handle_interrupt);
}

static void rx_callback(sx127x* device, uint8_t* data, uint16_t data_length) {
    if (data_length < 5) {
        printf("ERROR: data of size %d is too small\n", data_length);
        return;
    }

    packet_t* packet = malloc(256);
    if (data_length - sizeof(header_t) > 255) {
        printf("ERRRRORRR\n");
    }
    /* printf("Message id %d\n", *(uint16_t*)&data[1]); */
    memcpy(packet->data, data + sizeof(header_t), data_length - sizeof(header_t));
    packet->header.data_length = data_length - sizeof(header_t);
    packet->header.sender_id = data[0];
    packet->header.message_id = *(uint16_t*)&data[1];
    packet->header.command = data[4];
    add_message(packet);

    sx127x_set_opmod(SX127x_MODE_CAD, SX127x_MODULATION_LORA, device);
}

static void cad_callback(sx127x* device, int cad_detected) {
    if (cad_detected == 0) {
        if (has_next_message()) {
            if (!waited) {
                vTaskDelay((rand() % 100) / portTICK_PERIOD_MS);
                waited = true;
                ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_CAD, SX127x_MODULATION_LORA, device));
            } else {
                waited = false;
                ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_TX, SX127x_MODULATION_LORA, device));
            }
        } else {
            ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_CAD, SX127x_MODULATION_LORA, device));
        }
        return;
    }

    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_RX_CONT, SX127x_MODULATION_LORA, device));
}

static void task_handler(void* arg) {
    while (1) {
        vTaskSuspend(NULL);
        sx127x_handle_interrupt((sx127x*)arg);
    }
}

int32_t init_lora() {
    receive_queue = xQueueCreate(MAX_QUEUE_SIZE, sizeof(packet_t*));
    send_queue = xQueueCreate(MAX_QUEUE_SIZE, sizeof(packet_t*));

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
    sx127x_rx_set_callback(rx_callback, device);
    sx127x_lora_cad_set_callback(cad_callback, device);

    BaseType_t task_code = xTaskCreatePinnedToCore(task_handler, "handle lora", 8196, device, 2, &handle_interrupt, 1);
    if (task_code != pdPASS) {
        sx127x_destroy(device);
        return -1;
    }

    gpio_set_direction((gpio_num_t)DIO0, GPIO_MODE_INPUT);
    gpio_pulldown_en((gpio_num_t)DIO0);
    gpio_pullup_dis((gpio_num_t)DIO0);
    gpio_set_intr_type((gpio_num_t)DIO0, GPIO_INTR_POSEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add((gpio_num_t)DIO0, handle_interrupt_fromisr, (void*)device);

    sx127x_tx_header_t header = {
        .enable_crc = true,
        .coding_rate = SX127x_CR_4_5,
    };
    ESP_ERROR_CHECK(sx127x_lora_tx_set_explicit_header(&header, device));

    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_CAD, SX127x_MODULATION_LORA, device));

    return 0;
}

static data_t get_next_message() {
    packet_t* packet = NULL;
    if (xQueueReceiveFromISR(send_queue, &packet, 0) != pdTRUE) {
        data_t data = {
            .data = NULL,
            .size = 0,
        };
        return data;
    }
    uint8_t data_size = sizeof(header_t) + packet->header.data_length;
    data_t data = {
        .data = (uint8_t*)packet,
        .size = data_size,
    };
    return data;
}

static bool has_next_message() {
    return uxQueueMessagesWaitingFromISR(send_queue) != 0;
}

static void add_message(packet_t* data) {
    if (xQueueSendFromISR(receive_queue, &data, 0) != pdTRUE) {
        pop_queue(&receive_queue);
        xQueueSendFromISR(receive_queue, &data, 0);
    }
}

static void pop_queue(QueueHandle_t* handle) {
    packet_t* data = NULL;
    xQueueReceiveFromISR(*handle, &data, 0);
    if (data != NULL) {
        free(data);
    }
}

int16_t send_message(uint8_t* message, uint8_t data_length) {
    packet_t* packet = create_packet(message, data_length);
    if (xQueueSendFromISR(send_queue, &packet, 0) != pdTRUE) {
        pop_queue(&send_queue);
        xQueueSendFromISR(send_queue, &packet, 0);
    }

    return packet->header.message_id;
}

bool has_message() {
    return uxQueueMessagesWaitingFromISR(receive_queue) != 0;
}

// returns data allocated by malloc
packet_t* get_message() {
    packet_t* data;
    if (xQueueReceiveFromISR(receive_queue, &data, 0) != pdTRUE) {
        return NULL;
    }
    return data;
}
