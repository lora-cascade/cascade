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

static void pop_queue(QueueHandle_t* handle);

static bool get_next_message(data_t* data);

static bool has_next_message();

static void add_message(packet_t* packet);

static void add_send_message(packet_t* data);

static void task_handler(void* arg);

static int32_t backoff_round = 0;

static SemaphoreHandle_t semaphore;

static bool got_data = false;

static int64_t timeout = 0;

static std::set<uint8_t> known_ids;

static std::map<uint8_t, uint16_t> last_messages;

static bool kill_status = true;

static int64_t get_wait_time() {
    int32_t upper_bound = 1;

    for (int i = 0; i < backoff_round; i++) {
        upper_bound *= 2;
    }
    int32_t wait_slots = (int)random() % upper_bound;
    // printf("WAIT SLOTS: %ld\n", wait_slots);

    return CSMA_SLOT_TIME_MS * (int64_t)wait_slots + 1;
}

static int64_t get_current_time_us() {
    struct timeval val;
    gettimeofday(&val, NULL);
    return (int64_t)val.tv_usec;
}

static void handle_cad(bool input) {
    if (input) {
        timeout = get_current_time_us();
        LoRa.receive();
        got_data = true;
        ESP_DRAM_LOGE("sx127x", "receiving\n");
    } else {
        got_data = false;
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
    if (size < sizeof(header_t)) {
        ESP_DRAM_LOGE("sx127x", "Error: bad packet");
        return;
    }
    packet_t packet;
    for (int i = 0; i < size; i++) {
        ((uint8_t*)&packet)[i] = LoRa.read();
    }
    switch (packet.header.command) {
        case SEND_MESSAGE: {
            // check if not repeated message
            if (last_messages.find(packet.header.sender_id) == last_messages.end() ||
                last_messages[packet.header.sender_id] < packet.header.message_id) {
                last_messages[packet.header.sender_id] = packet.header.message_id;
                add_message(&packet);
                add_send_message(&packet);
            }
            break;
        }
        case JOIN_NETWORK: {
            packet_t join_return = create_join_return(known_ids);
            add_send_message(&join_return);
            known_ids.insert(packet.header.sender_id);
            last_messages.erase(packet.header.sender_id);
            break;
        }
        case ACK_NETWORK: {
            parse_received_nodes(&packet);
            break;
        }
        case KILL_MESSAGE: {
            // not repeated message
            kill_packet_t* kill_packet = (kill_packet_t*)&packet;
            if (last_messages.find(packet.header.sender_id) == last_messages.end() ||
                last_messages[packet.header.sender_id] <= packet.header.message_id) {
                last_messages[packet.header.sender_id] = packet.header.message_id;
                kill_status = kill_packet->kill;
                add_send_message(&packet);
            }
            break;
        }
        case DIRECTED_MESSAGE: {
            // not repeated message
            if (last_messages.find(packet.header.sender_id) == last_messages.end() ||
                last_messages[packet.header.sender_id] <= packet.header.message_id) {
                last_messages[packet.header.sender_id] = packet.header.message_id;
                if (((directed_packet_t*)&packet)->receiver_id == get_device_id()) {
                    // we are the receiver so store it
                    add_message(&packet);
                } else if (packet.header.sender_id != get_device_id()) {
                    // we are not the receiver so pass it on
                    add_send_message(&packet);
                }
            } else {
            }
            break;
        }
    }
    got_data = false;
    LoRa.channelActivityDetection();
}

static void handle_send() {
    LoRa.idle();
    data_t data;
    bool result = get_next_message(&data);
    if (result) {
        if (!LoRa.beginPacket()) {
            add_message((packet_t*)&data.data);
            LoRa.channelActivityDetection();
            return;
        }
        LoRa.write(data.data, data.size);
        LoRa.endPacket();
    }
    LoRa.channelActivityDetection();
}

static void task_handler(void* arg) {
    while (1) {
        if (has_next_message()) {
            if (!got_data) {
                backoff_round = 0;
                handle_send();
            } else {
                if (timeout + 20000 < get_current_time_us()) {
                    got_data = false;
                }
                backoff_round++;
                vTaskDelay(pdMS_TO_TICKS(get_wait_time()+3));
            }
        }
        vTaskDelay(1);
    }
}

int32_t init_lora() {
    receive_queue = xQueueCreate(MAX_QUEUE_SIZE, sizeof(packet_t));
    send_queue = xQueueCreate(MAX_QUEUE_SIZE, sizeof(packet_t));
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

    BaseType_t task_code = xTaskCreatePinnedToCore(task_handler, "handle lora", 8196, NULL, 2, &handle_interrupt, 0);
    if (task_code != pdPASS) {
        return -1;
    }

    for (int i = 0; i < 5; i++) {
        packet_t join_return = create_join();
        add_send_message(&join_return);
    }

    return 0;
}

static uint8_t packet_size_adjustment(uint8_t command) {
    switch (command) {
        case KILL_MESSAGE:
        case DIRECTED_MESSAGE:
            return 1;
        default:
            return 0;
    }
}

static bool get_next_message(data_t* data) {
    packet_t packet;
    if (xQueueReceiveFromISR(send_queue, &packet, 0) != pdTRUE) {
        return false;
    }
    uint8_t size = sizeof(header_t) + packet.header.data_length + packet_size_adjustment(packet.header.command);
    memcpy(&data->data, &packet, size);
    data->size = size;

    return true;
}

int get_size() {
    return uxQueueMessagesWaitingFromISR(send_queue);
}

uint8_t get_message_count() {
    int count = uxQueueMessagesWaiting(receive_queue);
    if(count >= 255) {
        return 255;
    }
    return count;
}

static bool has_next_message() {
    return uxQueueMessagesWaitingFromISR(send_queue) != 0;
}

static void add_message(packet_t* data) {
    if (xQueueSendFromISR(receive_queue, data, 0) != pdTRUE) {
        pop_queue(&receive_queue);
        xQueueSendFromISR(receive_queue, data, 0);
    }
}

static void pop_queue(QueueHandle_t* handle) {
    packet_t data;
    xQueueReceiveFromISR(*handle, &data, 0);
}

static void add_send_message(packet_t* data) {
    if (xQueueSendFromISR(send_queue, data, 0) != pdTRUE) {
        pop_queue(&send_queue);
        xQueueSendFromISR(send_queue, data, 0);
    }
}

int16_t send_message(uint8_t* message, uint8_t data_length) {
    packet_t packet = create_packet(message, data_length);
    add_send_message(&packet);

    return packet.header.message_id;
}

int16_t send_directed_message(uint8_t* message, uint8_t data_length, uint8_t target) {
    packet_t packet = create_directed_packet(message, data_length, target);
    add_send_message(&packet);

    return packet.header.message_id;
}

void send_packet(packet_t* packet) {
    add_send_message(packet);
}

uint8_t get_devices(uint8_t* list) {
    int j = 0;
    for(uint8_t i : known_ids) {
        if(j >= 255)
            break;
        list[j] = i;
        j++;
    }
    return j;
}

bool has_message() {
    return uxQueueMessagesWaitingFromISR(receive_queue) != 0;
}

packet_t get_message() {
    packet_t data;
    if (xQueueReceive(receive_queue, &data, 0) != pdTRUE) {
        return {};
    }
    return data;
}

void set_kill_status(uint8_t status) {
    kill_status = status;
}

bool get_kill_status() {
    return kill_status;
}
