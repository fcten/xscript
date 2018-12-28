#ifndef __WBT_WEBSOCKET_H__
#define	__WBT_WEBSOCKET_H__

#ifdef	__cplusplus
extern "C" {
#endif

#include "../webit.h"
#include "../common/wbt_string.h"

/*
  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-+-+-------+-+-------------+-------------------------------+
 |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
 |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
 |N|V|V|V|       |S|             |   (if payload len==126/127)   |
 | |1|2|3|       |K|             |                               |
 +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
 |     Extended payload length continued, if payload len == 127  |
 + - - - - - - - - - - - - - - - +-------------------------------+
 |                               |Masking-key, if MASK set to 1  |
 +-------------------------------+-------------------------------+
 | Masking-key (continued)       |          Payload Data         |
 +-------------------------------- - - - - - - - - - - - - - - - +
 :                     Payload Data continued ...                :
 + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
 |                     Payload Data continued ...                |
 +---------------------------------------------------------------+
 */

typedef enum {
    STATE_PARSE_OPCODE = 0,
    STATE_PARSE_PAYLOAD_LENGTH,
    STATE_PARSE_EXTENDED_PAYLOAD_LENGTH,
    STATE_PARSE_MASK,
    STATE_PARSE_PAYLOAD
} wbt_websocket_parse_state_t;

typedef struct wbt_websocket_s {
    // 状态机状态
    wbt_websocket_parse_state_t state;

    // 缓冲区
    struct {
        unsigned char *buf;
        unsigned length;
        unsigned offset;
    } data;

    // 结构化的数据
    unsigned int fin:1;
    unsigned int mask:1;
    unsigned int opcode:4;
    
    unsigned int count:4;

    unsigned char mask_key[4];
    
    unsigned long long int payload_length;
    unsigned char *payload;
} wbt_websocket_t;

wbt_status wbt_websocket_parse(wbt_websocket_t *ws);
wbt_status wbt_websocket_generate(wbt_websocket_t *ws);

#ifdef	__cplusplus
}
#endif

#endif	/* __WBT_WEBSOCKET_H__ */