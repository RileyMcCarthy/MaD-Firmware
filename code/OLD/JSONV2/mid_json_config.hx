#ifndef MID_JSON_CONFIG_H
#define MID_JSON_CONFIG_H
//
// Created by Riley McCarthy on 25/04/24.
//
/**********************************************************************
 * Includes
 **********************************************************************/
#include "mid_json.h"
/**********************************************************************
 * Constants
 **********************************************************************/
#define MID_JSON_NOTIFICATINO_TYPE_SIZE 10
#define MID_JSON_NOTIFICATINO_MESSAGE_SIZE 100
/*********************************************************************
 * Macros
 **********************************************************************/

/**********************************************************************
 * Typedefs
 **********************************************************************/
typedef enum
{
    MID_JSON_CHANNEL_NOTIFICATION,
    MID_JSON_CHANNEL_COUNT,
} mid_json_channel_E;

// shoould recreate all structs for json here.
// this will allow for ex sample struct to be different from what is used in monitor
typedef struct
{
    char type[MID_JSON_NOTIFICATINO_TYPE_SIZE];
    char message[MID_JSON_NOTIFICATINO_MESSAGE_SIZE];
} mid_json_notification_S;
/**********************************************************************
 * Public Function Definitions
 **********************************************************************/

/**********************************************************************
 * End of File
 **********************************************************************/
#endif /* MID_JSON_CONFIG_H */
