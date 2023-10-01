#ifndef DATA_H_
#define DATA_H_

#include <stdint.h>

typedef struct {
    uint8_t* data;
    uint8_t size;
} data_t;

typedef struct {
    data_t* list;
    uint16_t size;
    uint16_t capacity;
} data_list;

#endif  // DATA_H_
