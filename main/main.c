#include <stdlib.h>
#include "example.h"
#include "lora.h"
#include "esp_random.h"

void app_main() {
    int32_t result = init_lora();
    srand(esp_random());
    printf("result %ld\n", result);
    example();
}
