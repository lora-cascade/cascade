#include <stdlib.h>
#include "lora_interface.h"
#include "esp_random.h"
#include "sender.h"

extern "C" void app_main() {
    int32_t result = init_lora();
    srand(esp_random());
    sender();
}
