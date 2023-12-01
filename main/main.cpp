#include <stdlib.h>
#include "esp_log.h"
#include "esp_random.h"
#include "lora_interface.h"
#include "freertos/task.h"
#include "serial.h"

void serial_task(void*) {
    handle_serial();
}

extern "C" void app_main() {
    esp_log_level_set("*", ESP_LOG_NONE);
    int32_t result = init_lora();
    if (result != 0) {
        ESP_LOGE("sx127x", "ERROR: Cannot initialize lora");
        return;
    }
    srand(esp_random());
    TaskHandle_t task_handle;
    BaseType_t task_code = xTaskCreatePinnedToCore(serial_task, "handle serial", 8196, NULL, 3, &task_handle, 0);
    while(1) {
        vTaskDelay(1);
    }
    // sender();
}
