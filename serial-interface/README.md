# Serial interface

The serial interface is a concurrently executing process that:

1. reads bytes from the serial port,
2. translates the bytes into a command,
3. sends the command to the radio controller process over a queue,
4. receives the result of the command from the radio controller over another queue,
5. translates the result into a sequence of bytes,
6. writes the bytes over the serial port.

It repeats this process until the device is powered off.

The serial interface process is implemented as a FreeRTOS Task
(https://www.freertos.org/taskandcr.html)
and makes use of FreeRTOS Queues
(https://www.freertos.org/Embedded-RTOS-Queues.html).

Documentation on UART with ESP32:
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/uart.html

## Protocol definition

https://docs.google.com/document/d/11GAniZGIQtGn67Gn8hsuBswVlcI47bLu2ta6np2zhGA/edit
