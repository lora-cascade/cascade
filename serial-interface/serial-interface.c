#include "serial-interface.h"

// note: blocks task until requested number of bytes are available
void readbytes(uart_port_t u, byte *data, size_t len)
{
    // wait until all bytes are available to read
    size_t available;
    while (1) {
        uart_get_buffered_data_len(u, &available);
        if (available >= len)
            break;
        taskYIELD(); // give up control to another task
    }
    
    uart_read_bytes(u, data, len, 0);
}

// pointless wrapper around uart_write_bytes
void writebytes(uart_port_t u, byte *data, size_t len)
{
    uart_write_bytes(u, data, len);
}

void initialize_uart(uart_port_t u)
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
    ESP_ERROR_CHECK(uart_param_config(u, &uart_config));
    // TODO: change pin numbers
    ESP_ERROR_CHECK(uart_set_pin(u, 4, 5, 18, 19));
    // Setup UART buffered IO with event queue
    const int uart_buffer_size = (1024 * 2);
    QueueHandle_t uart_queue;
    // Install UART driver using an event queue here
    ESP_ERROR_CHECK(uart_driver_install(u, uart_buffer_size,   \
                                        uart_buffer_size, 10, &uart_queue, 0));
}

/*
  The first parameter should be a UART port number to read from.
  The second should be a queue to put commands into.
*/
void read_commands_from_uart_task(void *params)
{
    uart_port_t u = (uart_port_t) *(params[0]);
    QueueHandle_t q = (QueueHandle_t) *(params[1]);
    
    while (1) {
        si_command c;

	// read bytes from serial port and parse into command struct
        readbytes(u, &c.type, 1);
        switch (c.type) {
        case SI_SEND_MESSAGE:
            readbytes(u, &c.contents.send_message.receiver_id, 1);
            readbytes(u, &c.contents.send_message.message.length, 1);
            readbytes(u,
                      c.contents.send_message.message.contents,
                      c.contents.send_message.message.length);
            break;
        case SI_POLL_FOR_MESSAGES:
        case SI_LIST_NETWORK_DEVICES:
        case SI_SEND_KILL_MESSAGE:
        case SI_LISTEN_FOR_KILL_MESSAGES:
        case SI_STOP_LISTENING_FOR_KILL_MESSAGES:
            break; // command consists of only the "type" byte
        default:
            break; // error
        }

        // put command in queue; block task until space is available
        xQueueSendToBack(q, &c, portMAX_DELAY);
        
    }
}

/*
  The first parameter should be a UART port number to write to.
  The second should be a queue to read replies from.
*/
void write_replies_to_uart_task(void *params)
{
    uart_port_t u = (uart_port_t) *(params[0]);
    QueueHandle_t q = (QueueHandle_t) *(params[1]);

    while (1) {
        si_reply r;
        
        // read reply from queue; block task until a reply is available
        r = xQueueReceive(q, &r, portMAX_DELAY);
        
        // serialize reply struct into bytes and write to serial port
        switch (r.type) {
        case SI_SEND_MESSAGE:
            writebytes(u, &r.contents.send_message.sent_successfully, 1);
            break;
        case SI_POLL_FOR_MESSAGES:
            writebytes(u, &r.contents.poll_for_messages.num_messages, 1);
            for (int i = 0; i < r.contents.poll_for_messages.num_messages; i++) {
                writebytes(u, &r.contents.poll_for_messages.messages[i].length, 1);
                writebytes(u,
                           r.contents.poll_for_messages.messages[i].contents,
                           r.contents.poll_for_messages.messages[i].length);
            }
            break;
        case SI_LIST_NETWORK_DEVICES:
            writebytes(u, &r.contents.list_network_devices.num_devices, 1);
            writebytes(u,
                       r.contents.list_network_devices.device_ids,
                       r.contents.list_network_devices.num_devices);
            break;
        case SI_SEND_KILL_MESSAGE:
            writebytes(u, &r.contents.send_kill_message.sent_successfully, 1);
            break;
        case SI_LISTEN_FOR_KILL_MESSAGES:
            writebytes(u, &r.contents.listen_for_kill_messages.kill_device, 1);
            break;
        case SI_STOP_LISTENING_FOR_KILL_MESSAGES:
            writebytes(u, &reply.contents.stop_listening_for_kill_messages.end_of_status_messages, 1);
            break;
        default:
            break; // error
        }
    }
}



