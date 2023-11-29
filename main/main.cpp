#include <stdlib.h>
#include "esp_log.h"
#include "lora_interface.h"
#include "esp_random.h"
#include "sender.h"

extern "C" void app_main() {
    int32_t result = init_lora();
    if(result != 0) {
        ESP_LOGE("sx127x", "ERROR: Cannot initialize lora");
        return;
    }
    srand(esp_random());
    sender();
}
