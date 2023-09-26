#ifndef DATA_H_
#define DATA_H_

#include <stdint.h>

typedef struct {
    uint8_t* data;
    uint8_t size;
} data;

typedef struct {
    data* list;
    uint16_t size;
    uint16_t capacity;
} data_list;

#endif  // DATA_H_
