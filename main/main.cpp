#include <stdlib.h>
#include "lora_interface.h"
#include "esp_random.h"
#include "example.h"

extern "C" void app_main() {
    int32_t result = init_lora();
    srand(esp_random());
    example();
}
