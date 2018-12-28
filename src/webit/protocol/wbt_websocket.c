#include "../common/wbt_error.h"
#include "../common/wbt_memory.h"
#include "wbt_websocket.h"

wbt_status wbt_websocket_parse(wbt_websocket_t *ws) {
    while(ws->data.offset < ws->data.length) {
        unsigned char ch = ws->data.buf[ws->data.offset];

        switch(ws->state) {
            case STATE_PARSE_OPCODE:
                switch(ch & 0xF0) {
                    case 0x80: // 消息完毕
                        ws->fin = 1;
                        break;
                    case 0x00: // 消息尚未结束
                        ws->fin = 0;
                        break;
                    default:
                        return WBT_ERROR;
                }
                
                ws->opcode = ch & 0x0F;
                switch(ws->opcode) {
                    case 0x0: // 继续帧
                    case 0x1: // 文本帧
                    case 0x2: // 二进制帧
                    case 0x8: // 结束帧
                    case 0x9: // ping
                    case 0xA: // pong
                        break;
                    default:
                        return WBT_ERROR;
                }

                ws->state = STATE_PARSE_PAYLOAD_LENGTH;
                break;
            case STATE_PARSE_PAYLOAD_LENGTH:
                ws->mask = ch >> 7;
                ws->payload_length = ch & 0x7F;
                
                if( ws->mask != 1 ) {
                    return WBT_ERROR;
                }
                
                if (ws->payload_length == 126) {
                    ws->count = 2;
                    ws->state = STATE_PARSE_EXTENDED_PAYLOAD_LENGTH;
                } else if (ws->payload_length == 127) {
                    ws->count = 8;
                    ws->state = STATE_PARSE_EXTENDED_PAYLOAD_LENGTH;
                } else {
                    ws->state = STATE_PARSE_MASK;
                }
                break;
            case STATE_PARSE_EXTENDED_PAYLOAD_LENGTH:
                ws->payload_length = (ws->payload_length << 8) + ch;
                ws->count --;
                
                if (ws->count == 0) {
                    ws->state = STATE_PARSE_MASK;
                }
                break;
            case STATE_PARSE_MASK:
                ws->mask_key[ws->count++] = ch;
                
                if (ws->count == 4) {
                    ws->state = STATE_PARSE_PAYLOAD;
                }
                break;
            case STATE_PARSE_PAYLOAD:
                if (ws->data.offset + ws->payload_length > ws->data.length) {
                    return WBT_AGAIN;
                }

                ws->payload = ws->data.buf + ws->data.offset;
                ws->data.offset += ws->payload_length;

                // 解码
                int i = 0;
                for(i = 0;i < ws->payload_length; i++) {
                    ws->payload[i] ^= ws->mask_key[i%4];
                }

                ws->state = STATE_PARSE_OPCODE;
                return WBT_OK;
            default:
                return WBT_ERROR;
        }

        ws->data.offset ++;
    }

    return WBT_AGAIN; // 数据不完整
}

wbt_status wbt_websocket_generate(wbt_websocket_t *ws) {
    if (!ws->data.buf) {
        ws->data.offset = 0;
        ws->data.length = 10;
        ws->data.buf = wbt_malloc(ws->data.length);
        if (!ws->data.buf) {
            return WBT_ERROR;
        }
    }

    ws->data.buf[0] = 0x80 & ws->opcode;

    if (ws->payload_length <= 125) {
        ws->data.buf[1] = ws->payload_length;
        ws->data.offset = 2;
    } else if (ws->payload_length <= 65535) {
        ws->data.buf[1] = 126;
        ws->data.buf[2] = ws->payload_length >> 8;
        ws->data.buf[3] = ws->payload_length;
        ws->data.offset = 4;
    } else {
        ws->data.buf[1] = 127;
        int i;
        unsigned long long length = ws->payload_length;
        for (i = 9; i > 1; i--) {
            ws->data.buf[i] = length;
            length >>= 8;
        }
        ws->data.offset = 10;
    }

    return WBT_OK;
}