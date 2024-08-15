#ifndef MID_JSON_H
#define MID_JSON_H
//
// Created by Riley McCarthy on 25/04/24.
//
/**********************************************************************
 * Includes
 **********************************************************************/
#include "mid_json_config.h"
#include "mjson.h"
#include "stdint.h"
/**********************************************************************
 * Constants
 **********************************************************************/

/*********************************************************************
 * Macros
 **********************************************************************/
#define MID_NOTIFICATION_CHANNEL_CONFIG_CREATE(decodeBuffer, encodeBuffer, jsonBuffer, jsonAttrs, jsonFormat, jsonFormatArgs) \
    {                                                                                                                         \
        decodeBuffer,                                                                                                         \
        encodeBuffer,                                                                                                         \
        jsonBuffer,                                                                                                           \
        jsonAttrs,                                                                                                            \
        jsonFormat,                                                                                                           \
        jsonFormatArgs,                                                                                                       \
    },
/**********************************************************************
 * Typedefs
 **********************************************************************/

typedef struct
{
    char *fmt;
    void *arg;
    uint32_t argSize;
} mid_json_format_S;

typedef struct
{
    char *fmt;
    void **args;
    uint32_t size;
} mid_json_encodeJson_S;

typedef struct
{
    void *data; // pointer to data that will be decoded
    const struct json_attr_t *jsonAttrs;
} mid_json_decodeJson_S;

typedef struct
{
    mid_json_encodeJson_S jsonEncoder;
    mid_json_decodeJson_S jsonDecoder;
} mid_notification_channel_config_S;

typedef struct
{
    mid_notification_channel_config_S channels[MID_JSON_CHANNEL_COUNT];
} mid_json_config_S;
/**********************************************************************
 * Public Function Definitions
 **********************************************************************/
bool mid_json_encodeJson(mid_json_channel_E ch, void *data, char *json, int jsonSize);
bool mid_json_decodeJson(mid_json_channel_E ch, void *data, char *json);
/**********************************************************************
 * End of File
 **********************************************************************/
#endif /* MID_JSON_H */
