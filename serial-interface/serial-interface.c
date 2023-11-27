#include "serial-interface.h"



#define UART UART_NUM_2

void serial_interface_task(void *parameters)
{
    // configure UART (based on example from ESP32 documentation)
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
        .rx_flow_ctrl_thresh = 122,
    };
    ESP_ERROR_CHECK(uart_param_config(BOARD_UART_NUM, &uart_config));
    // TODO: change pin numbers
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, 4, 5, 18, 19));
    // Setup UART buffered IO with event queue
    const int uart_buffer_size = (1024 * 2);
    QueueHandle_t uart_queue;
    // Install UART driver using an event queue here
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2, uart_buffer_size,   \
                                        uart_buffer_size, 10, &uart_queue, 0));

    
    while (1) {
        si_command command;
        si_reply reply;
        
	// read bytes from serial port and parse into command struct
        command.type = getbyte();
        switch (command.type) {
        case SI_SEND_MESSAGE:
            command.contents.send_message.receiver_id = getbyte();
            command.contents.send_message.message.length = getbyte();
            getbytes(command.contents.send_message.message.contents,
                     command.contents.send_message.message.length);
            break;
        case SI_POLL_FOR_MESSAGES:
        case SI_LIST_NETWORK_DEVICES:
        case SI_SEND_KILL_MESSAGE:
        case SI_LISTEN_FOR_KILL_MESSAGES:
        case SI_STOP_LISTENING_FOR_KILL_MESSAGES:
            break; // command consists of only the "type" byte
        }

        // send command struct over queue to LoRa task

        // get reply struct over queue from LoRa task

        // serialize reply struct into bytes and write to serial port
        switch (reply.type) {
        case SI_SEND_MESSAGE:
            putbyte(reply.contents.send_message.sent_successfully);
            break;
        case SI_POLL_FOR_MESSAGES:
            putbyte(reply.contents.poll_for_messages.num_messages);
            for (int i = 0;
                 i < reply.contents.poll_for_messages.num_messages;
                 i++)
            {
                putbyte(reply.contents.poll_for_messages.messages[i].length);
                putbytes(reply.contents.poll_for_messages.messages[i].contents,
                         reply.contents.poll_for_messages.messages[i].length);
            }
            break;
        case SI_LIST_NETWORK_DEVICES:
            putbyte(reply.contents.list_network_devices.num_devices);
            putbytes(reply.contents.list_network_devices.device_ids,
                     reply.contents.list_network_devices.num_devices);
            break;
        case SI_SEND_KILL_MESSAGE:
            putbyte(reply.contents.send_kill_message.sent_successfully);
            break;
        case SI_LISTEN_FOR_KILL_MESSAGES:
            putbyte(reply.contents.listen_for_kill_messages.kill_device);
            break;
        case SI_STOP_LISTENING_FOR_KILL_MESSAGES:
            putbyte(reply.contents.stop_listening_for_kill_messages.end_of_status_messages);
            break;
        }
    }
}


void getbytes(byte *data, size_t len)
{
    while (uart_read_bytes(UART, data, len, 1000) == -1)
        ; // if timeout occurs, try again
}

byte getbyte()
{
    byte b;
    getbytes(&b, 1);
    return b;
}

void putbytes(byte *data, size_t len)
{
    uart_write_bytes(UART, data, len);
}

void putbyte(byte b)
{
    putbytes(&b, 1);
}

