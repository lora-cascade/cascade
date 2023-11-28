#include "lora_interface.h"
#include <stdint.h>
#include <algorithm>
#include <deque>
#include <map>
#include <set>
#include "command.h"
#include "esp_log.h"
#include "freertos/portable.h"
#include "freertos/portmacro.h"
#include "freertos/projdefs.h"
#include "packet.h"

static TaskHandle_t handle_interrupt;

static QueueHandle_t receive_queue;

static QueueHandle_t send_queue;

static int64_t time_finished;

static void pop_queue(QueueHandle_t* handle);

static data_t get_next_message();

static bool has_next_message();

static void add_message(packet_t* packet);

static void add_send_message(packet_t* data);

static void task_handler(void* arg);

static int32_t backoff_round = 0;

static SemaphoreHandle_t semaphore;

static BaseType_t wakeTask = pdFALSE;

static bool got_data = false;

static int64_t timeout = 0;

static bool send_join = false;

static int join_counter = 0;

static std::set<uint8_t> known_ids;

static std::map<uint8_t, uint16_t> last_messages;

static int64_t get_wait_time() {
    int32_t upper_bound = 1;

    for (int i = 0; i < backoff_round; i++) {
        upper_bound *= 2;
    }
    int32_t wait_slots = (int)LoRa.random() % upper_bound;
    printf("WAIT SLOTS: %ld\n", wait_slots);

    return CSMA_SLOT_TIME_MS * (int64_t)wait_slots + 1;
}

static int64_t get_current_time_us() {
    struct timeval val;
    gettimeofday(&val, NULL);
    return (int64_t)val.tv_usec;
}

static void handle_cad(bool input) {
    if (input) {
        // xSemaphoreTakeFromISR(semaphore, &wakeTask);
        timeout = get_current_time_us();
        LoRa.receive();
        got_data = true;
        ESP_DRAM_LOGE("sx127x", "receiving\n");
    } else {
        // xSemaphoreGiveFromISR(semaphore, &wakeTask);
        got_data = false;
        // ESP_DRAM_LOGE("sx127x", "listening\n");
        LoRa.channelActivityDetection();
    }
}

static void parse_received_nodes(packet_t* packet) {
    for (int i = 0; i < packet->header.data_length; i++) {
        uint8_t data = packet->data[i];
        if (data != get_device_id())
            known_ids.insert(data);
    }
    known_ids.insert(packet->header.sender_id);
}

static void handle_receive(int size) {
    /* printf("receiving data\n"); */
    if(size < sizeof(header_t)) {
        ESP_DRAM_LOGE("sx127x", "Error: bad packet");
    }
    packet_t* packet = (packet_t*)malloc(size);
    for (int i = 0; i < size; i++) {
        ((uint8_t*)packet)[i] = LoRa.read();
    }
    switch (packet->header.command) {
        case SEND_MESSAGE: {
            // check if not repeated message
            if (last_messages.find(packet->header.sender_id) == last_messages.end() ||
                last_messages[packet->header.sender_id] < packet->header.message_id) {
                last_messages[packet->header.sender_id] = packet->header.message_id;
                add_message(packet);
                add_send_message(packet);
            }
            break;
        }
        case JOIN_NETWORK: {
            ESP_DRAM_LOGE("lora", "new node %d\n", packet->header.sender_id);
            packet_t* join_return = create_join_return(known_ids);
            add_send_message(join_return);
            known_ids.insert(packet->header.sender_id);
            break;
        }
        case ACK_NETWORK: {
            ESP_DRAM_LOGE("lora", "new node %d with known %d\n", packet->header.sender_id, packet->header.data_length);
            parse_received_nodes(packet);
            break;
        }
        case DIRECTED_MESSAGE: {
            // not repeated message
            if (last_messages.find(packet->header.sender_id) == last_messages.end() ||
                last_messages[packet->header.sender_id] < packet->header.message_id) {
                last_messages[packet->header.sender_id] = packet->header.message_id;
                if (((directed_packet_t*)packet)->receiver_id == get_device_id()) {
                    // we are the receiver so store it
                    add_message(packet);
                } else if (packet->header.sender_id != get_device_id()) {
                    // we are not the receiver so pass it on
                    ESP_DRAM_LOGE("lora", "passing message from %d\n", packet->header.sender_id);
                    add_send_message(packet);
                }
            }
            break;
        }
    }
    got_data = false;
    // xSemaphoreGiveFromISR(semaphore, &wakeTask);
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
        if (((packet_t*)data.data)->header.command == ACK_NETWORK) {
            printf("sending ack\n");
        }
        LoRa.write(data.data, data.size);
        LoRa.endPacket();
        free(data.data);
    } else {
        printf("why null\n");
    }
    // xSemaphoreGive(semaphore);
    LoRa.channelActivityDetection();
}

static void task_handler(void* arg) {
    while (1) {
        if (has_next_message()) {
            if (!got_data) {  // xSemaphoreTake(semaphore, 0) == pdTRUE) {  //&& !got_data) {
                backoff_round = 0;
                handle_send();
            } else {
                if (timeout + 20000 < get_current_time_us()) {
                    got_data = false;
                }
                backoff_round++;
                vTaskDelay((get_wait_time() + 3) / portTICK_PERIOD_MS);
            }
        }
        vTaskDelay(1);
    }
}

int32_t init_lora() {
    receive_queue = xQueueCreate(MAX_QUEUE_SIZE, sizeof(packet_t*));
    send_queue = xQueueCreate(MAX_QUEUE_SIZE, sizeof(packet_t*));
    semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(semaphore);

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

    for (int i = 0; i < 5; i++) {
        packet_t* join_return = create_join();
        add_send_message(join_return);
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

static void add_send_message(packet_t* data) {
    if (xQueueSendFromISR(send_queue, &data, 0) != pdTRUE) {
        pop_queue(&send_queue);
        xQueueSendFromISR(send_queue, &data, 0);
    }
}

int16_t send_message(uint8_t* message, uint8_t data_length) {
    packet_t* packet = create_packet(message, data_length);
    add_send_message(packet);

    return packet->header.message_id;
}

int16_t send_directed_message(uint8_t* message, uint8_t data_length, uint8_t target) {
    packet_t* packet = create_directed_packet(message, data_length, target);
    add_send_message(packet);

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
