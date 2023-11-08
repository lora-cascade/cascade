#include "lora_interface.h"
#include <stdint.h>
#include "esp_log.h"
#include "freertos/portmacro.h"
#include "freertos/projdefs.h"
#include "packet.h"
#include <deque>
#include <algorithm>

static TaskHandle_t handle_interrupt;

static QueueHandle_t receive_queue;

static QueueHandle_t send_queue;

static int64_t time_finished;

static void pop_queue(QueueHandle_t* handle);

static data_t get_next_message();

static bool has_next_message();

static void add_message(packet_t* packet);

static void task_handler(void* arg);

static int32_t backoff_round = 0;

static SemaphoreHandle_t semaphore;

static BaseType_t wakeTask = pdFALSE;

static bool got_data = false;

static int64_t get_wait_time() {
    int32_t upper_bound = 1;

    for (int i = 0; i < backoff_round; i++) {
        upper_bound *= 2;
    }
    int32_t wait_slots = (int)LoRa.random() % upper_bound;
    printf("WAIT SLOTS: %ld\n", wait_slots);

    return CSMA_SLOT_TIME_MS * (int64_t)wait_slots + 1;
}

static void handle_cad(bool input) {
    if (input) {
        LoRa.receive();
        got_data = true;
        // ESP_DRAM_LOGE("sx127x", "receiving\n");
    } else {
        xSemaphoreGiveFromISR(semaphore, &wakeTask);
        LoRa.channelActivityDetection();
        portYIELD_FROM_ISR(wakeTask);
    }
}

static int64_t get_current_time_us() {
    struct timeval val;
    gettimeofday(&val, NULL);
    return (int64_t)val.tv_usec;
}

static void handle_receive(int size) {
    /* printf("receiving data\n"); */
    // ESP_DRAM_LOGE("sx127x", "%d\n", size);
    xSemaphoreTakeFromISR(semaphore, &wakeTask);
    packet_t* packet = (packet_t*)malloc(size);
    for (int i = 0; i < size; i++) {
        ((uint8_t*)packet)[i] = LoRa.read();
    }
    add_message(packet);
    got_data = false;
    xSemaphoreGiveFromISR(semaphore, &wakeTask);
    LoRa.channelActivityDetection();
}

static bool timeout_done() {
    printf("done\n");
    return time_finished - get_current_time_us() < 5;
}

static void handle_send() {
    // printf("sending message\n");
    LoRa.idle();
    data_t data = get_next_message();
    if (data.data != NULL) {
        if (!LoRa.beginPacket()) {
            add_message((packet_t*)data.data);
            LoRa.channelActivityDetection();
            return;
        }
        LoRa.write(data.data, data.size);
        LoRa.endPacket();
        free(data.data);
    } else {
        printf("why null\n");
    }
    LoRa.channelActivityDetection();
}

static void task_handler(void* arg) {
    while (1) {
        if (has_next_message()) {
            if(xSemaphoreTake(semaphore, 0) == pdTRUE && !got_data) {
                backoff_round = 0;
                handle_send();
            } else {
                backoff_round++;
                vTaskDelay(get_wait_time() / portTICK_PERIOD_MS);
            }
        }
        vTaskDelay(1);
    }
}

int32_t init_lora() {
    receive_queue = xQueueCreate(MAX_QUEUE_SIZE, sizeof(packet_t*));
    send_queue = xQueueCreate(MAX_QUEUE_SIZE, sizeof(packet_t*));
    semaphore = xSemaphoreCreateBinary();

    SPI.begin(5, 19, 27);
    LoRa.setPins(18, 23, 26);

    if (LoRa.begin(FREQUENCY) == 0) {
        return -1;
    }

    LoRa.setSyncWord(SYNCWORD);
    LoRa.enableCrc();

    LoRa.onCadDone(handle_cad);
    LoRa.onReceive(handle_receive);
    LoRa.channelActivityDetection();

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
