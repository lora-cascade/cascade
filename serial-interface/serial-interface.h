
#define SI_FALSE                  0x00
#define SI_TRUE                   0x01
#define SI_END_OF_STATUS_MESSAGES 0x02

#define SI_SEND_MESSAGE                     0x00
#define SI_POLL_FOR_MESSAGES                0x01
#define SI_LIST_NETWORK_DEVICES             0x02
#define SI_SEND_KILL_MESSAGE                0x03
#define SI_LISTEN_FOR_KILL_MESSAGES         0x04
#define SI_STOP_LISTENING_FOR_KILL_MESSAGES 0x05

#define SI_MAX_MESSAGE_LENGTH 255

typedef struct {
    unsigned char receiver_id;
    unsigned char length;
    unsigned char contents[SI_MAX_MESSAGE_LENGTH];
} si_message;

