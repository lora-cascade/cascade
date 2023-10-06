#include "lora_interface.h"
#include <stdint.h>
#include "freertos/projdefs.h"
#include "packet.h"

static TaskHandle_t handle_interrupt;

static QueueHandle_t receive_queue;

static QueueHandle_t send_queue;

static void pop_queue(QueueHandle_t* handle);

static data_t get_next_message();

static bool has_next_message();

static void add_message(packet_t* packet);

static void task_handler(void* arg);

bool waited = false;

static void handle_receive() {
    packet_t* packet = malloc(256);
    lora_receive_packet((uint8_t*)packet, 256);
    add_message(packet);
}

static void handle_send() {
    printf("sending message\n");
    vTaskDelay(100);
    data_t data = get_next_message();
    if (data.data != NULL) {
        lora_send_packet(data.data, data.size);
        free(data.data);
    } else {
        printf("why null\n");
    }
}

static void task_handler(void* arg) {
    while (1) {
        lora_receive();
        if (lora_received()) {
            handle_receive();
        } else if (has_next_message()) {
            handle_send();
        }
        vTaskDelay(1);
    }
}

int32_t init_lora() {
    receive_queue = xQueueCreate(MAX_QUEUE_SIZE, sizeof(packet_t*));
    send_queue = xQueueCreate(MAX_QUEUE_SIZE, sizeof(packet_t*));

    if (lora_init() == 0) {
        return -1;
    }

#if CONFIG_169MHZ
    ESP_LOGI(pcTaskGetName(NULL), "Frequency is 169MHz");
    lora_set_frequency(169e6);  // 169MHz
#elif CONFIG_433MHZ
    ESP_LOGI(pcTaskGetName(NULL), "Frequency is 433MHz");
    lora_set_frequency(433e6);  // 433MHz
#elif CONFIG_470MHZ
    ESP_LOGI(pcTaskGetName(NULL), "Frequency is 470MHz");
    lora_set_frequency(470e6);  // 470MHz
#elif CONFIG_866MHZ
    ESP_LOGI(pcTaskGetName(NULL), "Frequency is 866MHz");
    lora_set_frequency(866e6);  // 866MHz
#elif CONFIG_915MHZ
    ESP_LOGI(pcTaskGetName(NULL), "Frequency is 915MHz");
    lora_set_frequency(915e6);  // 915MHz
#elif CONFIG_OTHER
    ESP_LOGI(pcTaskGetName(NULL), "Frequency is %dMHz", CONFIG_OTHER_FREQUENCY);
    long frequency = CONFIG_OTHER_FREQUENCY * 1000000;
    lora_set_frequency(frequency);
#endif
    lora_enable_crc();

    lora_set_coding_rate(CODING_RATE);
    lora_set_bandwidth(BANDWIDTH);
    lora_set_spreading_factor(SPREADING_FACTOR);
    lora_set_sync_word(SYNCWORD);

    BaseType_t task_code = xTaskCreatePinnedToCore(task_handler, "handle lora", 8196, NULL, 2, &handle_interrupt, 1);
    if (task_code != pdPASS) {
        return -1;
    }

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

int get_size() {
    return uxQueueMessagesWaitingFromISR(send_queue);
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
