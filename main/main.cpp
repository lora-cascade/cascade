#include <stdlib.h>
#include "lora_interface.h"
#include "esp_random.h"
#include "sender.h"

#define QUEUE_LENGTH 8

extern "C" void app_main() {
    int32_t result = init_lora();
    srand(esp_random());
    sender();

    // serial-interface task initialization
    QueueHandle_t command_queue = xQueueCreate(QUEUE_LENGTH, sizeof(si_command));
    QueueHandle_t reply_queue = xQueueCreate(QUEUE_LENGTH, sizeof(si_reply));
}
