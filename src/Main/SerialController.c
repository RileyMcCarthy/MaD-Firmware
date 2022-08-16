#include "SerialController.h"

#include <stdio.h>
#include <stdarg.h>

#define MESSAGE_TYPES 7
typedef enum messageTypes_e
{
    MESSAGE_TYPE_NONE = 0,
    MESSAGE_TYPE_DEBUG,
    MESSAGE_TYPE_NOTIFY,
    MESSAGE_TYPE_WARNING,
    MESSAGE_TYPE_ERROR,
    MESSAGE_TYPE_STATUS,
    MESSAGE_TYPE_DATA
} MessageType;

static char *messageTypes[] = {"NONE", "DEBUG", "NOTIFY", "WARNING", "ERROR", "STATUS", "DATA"};

typedef struct message_s
{
    MessageType type;
    char message[256];
} Message;

#define SERIAL_MEMORY_SIZE 100
static long serial_stack[SERIAL_MEMORY_SIZE];

#define MESSAGE_QUEUE_SIZE 10
static Message queue[10];
static int queue_head = 0;

#define serial_debug(format, ...) serial_cmd(MESSAGE_TYPE_DEBUG, format, __VA_ARGS__)
#define serial_error(format, ...) serial_cmd(MESSAGE_TYPE_ERROR, format, __VA_ARGS__)
#define serial_notify(format, ...) serial_cmd(MESSAGE_TYPE_NOTIFY, format, __VA_ARGS__)

static void remove_queue()
{
    // Remove message from queue
    for (int i = 0; i < queue_head - 1; i++)
    {
        queue[i] = queue[i + 1];
    }
    queue_head--;
}

void serial_cmd(MessageType type, char *format, ...)
{
    long startms = _getms();
    while (queue_head >= MESSAGE_QUEUE_SIZE)
    {
        if (_getms() - startms > 1000)
        {
            break;
        }
    } // wait for queue to empty
    va_list args;
    va_start(args, format);
    queue[queue_head].type = type;
    vsprintf(queue[queue_head].message, format, args);
    va_end(args);
    queue_head++;
}

static void serial_cog(void *arg)
{
    while (1)
    {
        if (queue_head != 0)
        {
            // Send message
            Message message = queue[0];
            if (message.type < MESSAGE_TYPES)
            {
                printf("<%s>%s</%s>\n", messageTypes[message.type], message.message, messageTypes[message.type]);
            }
            else
            {
                serial_error("Invalid message type:%d,%s\n", message.type, message.message);
            }
            // Remove message from queue
            remove_queue();
        }
        Message message;
        char cmd[10];
        scanf("<%s>%s</%s>", cmd, message.message, cmd);
        if (strcmp(cmd, messageTypes[MESSAGE_TYPE_STATUS]) == 0)
        {
            // Send status
        }
        else if (strcmp(cmd, messageTypes[MESSAGE_TYPE_DATA]) == 0)
        {
            // Send data
        }
        else
        {
            serial_notify("Unknown command: %s", cmd);
        }
    }
}

bool serial_begin(void)
{
    serial_debug("Serial begin\n");
    int cogid = _cogstart_C(serial_cog, NULL, &serial_stack[0], sizeof(long) * SERIAL_MEMORY_SIZE);
    if (cogid != -1)
    {
        return true;
    }
}