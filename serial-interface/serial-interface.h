
/*

  Design based on the concept of a "tagged union" (https://en.wikipedia.org/wiki/Tagged_union).
  
*/

// Reply symbols
#define SI_FALSE                  0x00
#define SI_TRUE                   0x01
#define SI_END_OF_STATUS_MESSAGES 0x02

// Command/reply types
#define SI_SEND_MESSAGE                     0x00
#define SI_POLL_FOR_MESSAGES                0x01
#define SI_LIST_NETWORK_DEVICES             0x02
#define SI_SEND_KILL_MESSAGE                0x03
#define SI_LISTEN_FOR_KILL_MESSAGES         0x04
#define SI_STOP_LISTENING_FOR_KILL_MESSAGES 0x05

#define SI_MAX_MESSAGE_LENGTH 255

typedef (unsigned char) byte;

typedef struct {
  byte length;
  byte contents[SI_MAX_MESSAGE_LENGTH];
} lora_message;

typedef struct {
  byte type;
  union {
    struct {
      byte receiver_id;
      lora_message message;
    } send_message;
    // command types not listed here have empty contents
  } contents;
} si_command;

typedef struct {
  byte type;
  union {
    struct { byte sent_successfully; } send_message;
    struct {
      byte num_messages;
      lora_message *messages;
    } poll_for_messages;
    struct {
      byte num_devices;
      byte *device_ids;
    } list_network_devices;
    struct { byte sent_successfully; } send_kill_message;
    struct { byte kill_device; } listen_for_kill_messages;
    struct { byte end_of_status_messages; } stop_listening_for_kill_messages;
  } contents;
} si_reply;

void initialize_uart(uart_port_t u);
void read_commands_from_uart_task(void *params);
void write_replies_to_uart_task(void *params);
