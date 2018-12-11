#include "../common/wbt_error.h"
#include "../common/wbt_memory.h"
#include "wbt_http.h"

wbt_str_t header_server = wbt_string("BitMQ");
wbt_str_t header_cache_control = wbt_string("max-age=3600");
wbt_str_t header_cache_control_no_cache = wbt_string("no-store, no-cache, must-revalidate");
wbt_str_t header_pragma_no_cache = wbt_string("no-cache");
wbt_str_t header_expires_no_cache = wbt_string("Sat, 26 Jul 1997 05:00:00 GMT");
wbt_str_t header_encoding_gzip = wbt_string("gzip");

wbt_str_t REQUEST_METHOD[] = {
    wbt_null_string,
    wbt_string("GET"),
    wbt_string("POST"),
    wbt_string("HEAD"),
    wbt_string("PUT"), 
    wbt_string("DELETE"), 
    wbt_string("TRACE"),
    wbt_string("CONNECT"),
    wbt_string("OPTIONS"),
    wbt_null_string
};

wbt_str_t HTTP_HEADERS[] = {
    wbt_null_string,
    wbt_string("Cache-Control"),
    wbt_string("Connection"),
    wbt_string("Date"),
    wbt_string("Pragma"),
    wbt_string("Trailer"),
    wbt_string("Transfer-Encoding"),
    wbt_string("Upgrade"),
    wbt_string("Via"),
    wbt_string("Warning"),
    wbt_string("Accept"),
    wbt_string("Accept-Charset"),
    wbt_string("Accept-Encoding"),
    wbt_string("Accept-Language"),
    wbt_string("Authorization"),
    wbt_string("Expect"),
    wbt_string("From"),
    wbt_string("Host"),
    wbt_string("If-Match"),
    wbt_string("If-Modified-Since"),
    wbt_string("If-None-Match"),
    wbt_string("If-Range"),
    wbt_string("If-Unmodified-Since"),
    wbt_string("Max-Forwards"),
    wbt_string("Proxy-Authorization"),
    wbt_string("Range"),
    wbt_string("Referer"),
    wbt_string("TE"),
    wbt_string("User-Agent"),
    wbt_string("Accept-Ranges"),
    wbt_string("Age"),
    wbt_string("Etag"),
    wbt_string("Location"),
    wbt_string("Proxy-Autenticate"),
    wbt_string("Retry-After"),
    wbt_string("Server"),
    wbt_string("Vary"),
    wbt_string("WWW-Authenticate"),
    wbt_string("Allow"),
    wbt_string("Content-Encoding"),
    wbt_string("Content-Language"),
    wbt_string("Content-Length"),
    wbt_string("Content-Location"),
    wbt_string("Content-MD5"),
    wbt_string("Content-Range"),
    wbt_string("Content-Type"),
    wbt_string("Expires"),
    wbt_string("Last-Modified"),
    wbt_string("Cookie"),
    wbt_string("Set-Cookie"),
    wbt_string("Access-Control-Allow-Origin"),
#ifdef WITH_WEBSOCKET
    wbt_string("Sec-WebSocket-Key"),
    wbt_string("Sec-WebSocket-Version"),
    wbt_string("Sec-WebSocket-Protocol"),
    wbt_string("Sec-WebSocket-Extensions"),
    wbt_string("Sec-WebSocket-Accept"),
#endif
    wbt_null_string
};

wbt_str_t STATUS_CODE[] = {
    wbt_null_string,
    wbt_string("100"),
    wbt_string("101"), 
    wbt_string("102"),
    wbt_string("200"),
    wbt_string("201"),
    wbt_string("202"),
    wbt_string("203"),
    wbt_string("204"),
    wbt_string("205"),
    wbt_string("206"),
    wbt_string("207"),
    wbt_string("300"),
    wbt_string("301"),
    wbt_string("302"),
    wbt_string("303"),
    wbt_string("304"),
    wbt_string("305"),
    wbt_string("307"),
    wbt_string("400"),
    wbt_string("401"),
    wbt_string("402"),
    wbt_string("403"),
    wbt_string("404"),
    wbt_string("405"),
    wbt_string("406"),
    wbt_string("407"),
    wbt_string("408"),
    wbt_string("409"),
    wbt_string("410"),
    wbt_string("411"),
    wbt_string("412"),
    wbt_string("413"),
    wbt_string("414"),
    wbt_string("415"),
    wbt_string("416"),
    wbt_string("417"),
    wbt_string("422"),
    wbt_string("423"),
    wbt_string("424"),
    wbt_string("426"),
    wbt_string("500"),
    wbt_string("501"),
    wbt_string("502"),
    wbt_string("503"),
    wbt_string("504"),
    wbt_string("505"),
    wbt_string("506"),
    wbt_string("507"),
    wbt_string("510"),
    wbt_null_string
};

wbt_str_t STATUS_MESSAGE[] = {
    wbt_null_string,
    wbt_string("Continue"),
    wbt_string("Switching Protocols"), 
    wbt_string("Processing"),
    wbt_string("OK"),
    wbt_string("Created"),
    wbt_string("Accepted"),
    wbt_string("Non-Authoritative Information"),
    wbt_string("No Content"),
    wbt_string("Reset Content"),
    wbt_string("Partial Content"),
    wbt_string("Multi-Status"),
    wbt_string("Multiple Choices"),
    wbt_string("Moved Permanently"),
    wbt_string("Found"),
    wbt_string("See Other"),
    wbt_string("Not Modified"),
    wbt_string("Use Proxy"),
    wbt_string("Temporary Redirect"),
    wbt_string("Bad Request"),
    wbt_string("Unauthorized"),
    wbt_string("Payment Required"),
    wbt_string("Forbidden"),
    wbt_string("Not Found"),
    wbt_string("Method Not Allowed"),
    wbt_string("Not Acceptable"),
    wbt_string("Proxy Authentication Required"),
    wbt_string("Request Time-out"),
    wbt_string("Conflict"),
    wbt_string("Gone"),
    wbt_string("Length Required"),
    wbt_string("Precondition Failed"),
    wbt_string("Request Entity Too Large"),
    wbt_string("Request-URI Too Large"),
    wbt_string("Unsupported Media Type"),
    wbt_string("Requested range not satisfiable"),
    wbt_string("Expectation Failed"),
    wbt_string("Unprocessable Entity"),
    wbt_string("Locked"),
    wbt_string("Failed Dependency"),
    wbt_string("Upgrade Required"),
    wbt_string("Internal Server Error"),
    wbt_string("Not Implemented"),
    wbt_string("Bad Gateway"),
    wbt_string("Service Unavailable"),
    wbt_string("Gateway Time-out"),
    wbt_string("HTTP Version not supported"),
    wbt_string("Variant Also Negotiates"),
    wbt_string("Insufficient Storage"),
    wbt_string("Not Extended"),
    wbt_null_string
};

const char *wbt_http_method(wbt_http_method_t method) {
    if (method > 0 && method < METHOD_LENGTH ) {
        return REQUEST_METHOD[method].str;
    }
    return REQUEST_METHOD[0].str;
}

wbt_status wbt_http_parse_request_body(wbt_http_request_t *req) {
    if (req->content_length > 0) {
        if (req->recv.length >= req->recv.offset + req->content_length) {
            req->body.len = req->content_length;
            req->recv.offset += req->content_length;
            return WBT_OK;
        } else {
            return WBT_AGAIN;
        }
    } else {
        req->body.len = 0;
        return WBT_OK;
    }
}

wbt_status wbt_http_parse_request_line(wbt_http_request_t *req) {
    while (req->recv.offset < req->recv.length) {
        char ch = req->recv.buf[req->recv.offset];

        switch (req->state) {
            case STATE_PARSE_REQUEST_LINE_START: {
                req->method_str.start = req->recv.offset;
                req->state = STATE_PARSE_REQUEST_LINE_METHOD;
                break;
            }
            case STATE_PARSE_REQUEST_LINE_METHOD: { // 解析 method
                if (ch == ' ') {
                    req->method_str.len = req->recv.offset - req->method_str.start;

                    wbt_str_t method;
                    wbt_offset_to_str(req->method_str, method, req->recv.buf);

                    int i;
                    for(i = METHOD_UNKNOWN + 1 ; i < METHOD_LENGTH ; i++ ) {
                        if( wbt_strncmp(&method,
                                &REQUEST_METHOD[i],
                                REQUEST_METHOD[i].len ) == 0 ) {
                            req->method = i;
                            break;
                        }
                    }
                    
                    if (req->method == METHOD_UNKNOWN) {
                        return WBT_ERROR;
                    } else {
                        req->state = STATE_PARSE_REQUEST_LINE_URI;
                        req->uri.start = req->recv.offset + 1;
                    }
                } else if (req->recv.offset >= 8) { // 长度限制
                    return WBT_ERROR;
                }
                break;
            }
            case STATE_PARSE_REQUEST_LINE_URI: { // 解析 uri
                if (ch == ' ') {
                    req->uri.len = req->recv.offset - req->uri.start;
                    req->state = STATE_PARSE_REQUEST_LINE_VERSION;
                    req->params.start = 0;
                    req->params.len = 0;
                    req->version_str.start = req->recv.offset + 1;
                } else if (ch == '?') {
                    req->uri.len = req->recv.offset - req->uri.start;
                    req->state = STATE_PARSE_REQUEST_LINE_PARAMS;
                    req->params.start = req->recv.offset + 1;
                }
                break;
            }
            case STATE_PARSE_REQUEST_LINE_PARAMS: { // 解析 param
                if (ch == ' ') {
                    req->params.len = req->recv.offset - req->params.start;
                    req->state = STATE_PARSE_REQUEST_LINE_VERSION;
                    req->version_str.start = req->recv.offset + 1;
                }
                break;
            }
            case STATE_PARSE_REQUEST_LINE_VERSION: { // 解析 http 版本号
                if (ch == '\r') {
                    req->version_str.len = req->recv.offset - req->version_str.start;

                    wbt_str_t ver;
                    wbt_str_t http_ver_1_0 = wbt_string("HTTP/1.0");
                    wbt_str_t http_ver_1_1 = wbt_string("HTTP/1.1");

                    wbt_offset_to_str(req->version_str, ver, req->recv.buf);

                    if (wbt_strcmp(&ver, &http_ver_1_0) == 0) {
                        req->version = PROTO_HTTP_1_0;
                    } else if (wbt_strcmp(&ver, &http_ver_1_1) == 0) {
                        req->version = PROTO_HTTP_1_1;
                    } else {
                        req->version = PROTO_HTTP_UNKNOWN;
                        return WBT_ERROR;
                    }

                    req->state = STATE_PARSE_REQUEST_LINE_END;
                }
                break;
            }
            case STATE_PARSE_REQUEST_LINE_END: {
                if (ch == '\n') {
                    req->state = STATE_PARSE_REQUEST_HEADER_START;
                    req->recv.offset ++;
                    req->headers.start = req->recv.offset;
                    req->header.key.start = req->recv.offset;
                    return WBT_OK;
                } else {
                    return WBT_ERROR;
                }
                break;
            }
            default:
                return WBT_ERROR;
        }

        req->recv.offset ++;
    }

    return WBT_AGAIN; // 数据不完整
}

wbt_status wbt_http_parse_request_header(wbt_http_request_t *req) {
    while (req->recv.offset < req->recv.length) {
        char ch = req->recv.buf[req->recv.offset];

        switch (req->state) {
            case STATE_PARSE_REQUEST_HEADER_START: {
                if (ch == '\r') {
                    req->state = STATE_PARSE_REQUEST_HEADERS_END;
                } else {
                    req->header.key.start = req->recv.offset;
                    req->state = STATE_PARSE_REQUEST_HEADER_KEY;
                }
                break;
            }
            case STATE_PARSE_REQUEST_HEADER_KEY: {
                if (ch == ':') {
                    req->header.key.len = req->recv.offset - req->header.key.start;
                    req->header.value.start = req->recv.offset + 1;
                    req->state = STATE_PARSE_REQUEST_HEADER_VALUE;
                }
                break;
            }
            case STATE_PARSE_REQUEST_HEADER_VALUE: {
                if (ch == '\r') {
                    while (req->recv.buf[req->header.value.start] == ' ') {
                        req->header.value.start ++;
                    }
                    req->header.value.len = req->recv.offset - req->header.value.start;
                    req->state = STATE_PARSE_REQUEST_HEADER_END;
                }
                break;
            }
            case STATE_PARSE_REQUEST_HEADER_END: {
                if (ch == '\n') {
                    req->state = STATE_PARSE_REQUEST_HEADER_START;
                    req->recv.offset ++;
                    return WBT_OK;
                } else {
                    return WBT_ERROR;
                }
                break;
            }
            case STATE_PARSE_REQUEST_HEADERS_END: {
                if (ch == '\n') {
                    req->headers.len = req->recv.offset - req->headers.start - 3;
                    req->state = STATE_PARSE_REQUEST_BODY;
                    req->recv.offset ++;
                    req->body.start = req->recv.offset;
                    return WBT_OK;
                } else {
                    return WBT_ERROR;
                }
                break;
            }
            default:
                return WBT_ERROR;
        }

        req->recv.offset ++;
    }

    return WBT_AGAIN; // 数据不完整
}

wbt_status wbt_http_parse_request(wbt_http_request_t *req) {
    wbt_status ret = WBT_OK;

    while (ret == WBT_OK) {
        if (req->state <= STATE_PARSE_REQUEST_LINE_END) {
            ret = wbt_http_parse_request_line(req);
        } else if (req->state <= STATE_PARSE_REQUEST_HEADERS_END) {
            ret = wbt_http_parse_request_header(req);
            if (ret == WBT_OK && req->state == STATE_PARSE_REQUEST_HEADER_START) {
                wbt_str_t header_key, header_value;
                wbt_offset_to_str(req->header.key, header_key, req->recv.buf);
                wbt_offset_to_str(req->header.value, header_value, req->recv.buf);

                wbt_debug("%.*s", header_key.len, header_key.str);

                if(wbt_strnicmp( &header_key, &HTTP_HEADERS[HEADER_CONNECTION], HTTP_HEADERS[HEADER_CONNECTION].len ) == 0) {
                    wbt_str_t keep_alive = wbt_string("keep-alive");
                    if( wbt_strnicmp( &header_value, &keep_alive, keep_alive.len ) == 0 ) {
                        /* 声明为 keep-alive 连接 */
                        req->keep_alive = 1;
                    } else {
                        req->keep_alive = 0;
                    }
                } else if (wbt_strnicmp( &header_key, &HTTP_HEADERS[HEADER_CONTENT_LENGTH], HTTP_HEADERS[HEADER_CONTENT_LENGTH].len ) == 0) {
                    req->content_length = wbt_atoi(&header_value);
                }
            }
        } else if (req->state <= STATE_PARSE_REQUEST_BODY) {
            return wbt_http_parse_request_body(req);
        } else {
            return WBT_ERROR;
        }
    }

    return ret;
}

wbt_status wbt_http_generate_response_check_space(wbt_http_response_t *resp, int len) {
    while (resp->send.length < resp->send.offset + len) {
        void *buf = wbt_realloc(resp->send.buf, resp->send.length * 2);
        if (!buf) {
            return WBT_ERROR;
        }
        resp->send.buf = (char *)buf;
        resp->send.length *= 2;
    }
    return WBT_OK;
}

wbt_status _wbt_http_generate_response_status(wbt_http_response_t *resp) {
    wbt_str_t proto = wbt_string("HTTP/1.1 ");
    wbt_memcpy(resp->send.buf + resp->send.offset, proto.str, proto.len);
    resp->send.offset += proto.len;

    if (resp->status > 0 && resp->status < STATUS_LENGTH) {
        wbt_memcpy(resp->send.buf + resp->send.offset, STATUS_CODE[resp->status].str, STATUS_CODE[resp->status].len);
        resp->send.offset += STATUS_CODE[resp->status].len;

        resp->send.buf[resp->send.offset++] = ' ';

        if (resp->message.str && resp->message.len > 0 && resp->message.len < 128) {
            wbt_memcpy(resp->send.buf + resp->send.offset, resp->message.str, resp->message.len);
            resp->send.offset += resp->message.len;
        } else {
            wbt_memcpy(resp->send.buf + resp->send.offset, STATUS_MESSAGE[resp->status].str, STATUS_MESSAGE[resp->status].len);
            resp->send.offset += STATUS_MESSAGE[resp->status].len;
        }

        resp->send.buf[resp->send.offset++] = '\r';
        resp->send.buf[resp->send.offset++] = '\n';

        return WBT_OK;
    } else {
        return WBT_ERROR;
    }
}

wbt_status wbt_http_generate_response_header(wbt_http_response_t *resp, wbt_http_line_t key, wbt_str_t *value) {
    if (key > 0 && key < HEADER_LENGTH) {
        int len = HTTP_HEADERS[key].len + value->len + 4;
        if (wbt_http_generate_response_check_space(resp, len) != WBT_OK) {
            return WBT_ERROR;
        }

        wbt_memcpy(resp->send.buf + resp->send.offset, HTTP_HEADERS[key].str, HTTP_HEADERS[key].len);
        resp->send.offset += HTTP_HEADERS[key].len;

        resp->send.buf[resp->send.offset++] = ':';
        resp->send.buf[resp->send.offset++] = ' ';

        wbt_memcpy(resp->send.buf + resp->send.offset, value->str, value->len);
        resp->send.offset += value->len;

        resp->send.buf[resp->send.offset++] = '\r';
        resp->send.buf[resp->send.offset++] = '\n';

        return WBT_OK;
    } else {
        return WBT_ERROR;
    }
}

wbt_status wbt_http_generate_response_body(wbt_http_response_t *resp, wbt_str_t *body) {
    if (body->len < 0) {
        return WBT_ERROR;
    }

    char len[16];
    wbt_str_t content_length;
    content_length.str = len;
    content_length.len = sprintf(len, "%d", body->len);

    if (wbt_http_generate_response_header(resp, HEADER_CONTENT_LENGTH, &content_length) != WBT_OK) {
        return WBT_ERROR;
    }

    if (wbt_http_generate_response_check_space(resp, body->len + 2) != WBT_OK) {
        return WBT_ERROR;
    }

    resp->send.buf[resp->send.offset++] = '\r';
    resp->send.buf[resp->send.offset++] = '\n';

    wbt_memcpy(resp->send.buf + resp->send.offset, body->str, body->len);
    resp->send.offset += body->len;

    return WBT_OK;
}

wbt_status wbt_http_generate_response_status(wbt_http_response_t *resp) {
    resp->send.buf = (char *)wbt_malloc(4 * 1024);
    if (!resp->send.buf) {
        return WBT_ERROR;
    }
    resp->send.length = 4 * 1024;
    resp->send.offset = 0;

    return _wbt_http_generate_response_status(resp);
}

wbt_status wbt_http_parse_response_header(wbt_http_upsteam_response_t *resp) {
    while (resp->recv.offset < resp->recv.length) {
        char ch = resp->recv.buf[resp->recv.offset];

        switch (resp->state) {
            case 0: {
                if (resp->recv.length < resp->recv.offset + 9) {
                    return WBT_AGAIN;
                }

                wbt_str_t ver;
                wbt_str_t http_ver_1_0 = wbt_string("HTTP/1.0 ");
                wbt_str_t http_ver_1_1 = wbt_string("HTTP/1.1 ");

                ver.str = resp->recv.buf + resp->recv.offset;
                ver.len = http_ver_1_0.len;

                if (wbt_strcmp(&ver, &http_ver_1_0) == 0) {
                    resp->version = PROTO_HTTP_1_0;
                } else if (wbt_strcmp(&ver, &http_ver_1_1) == 0) {
                    resp->version = PROTO_HTTP_1_1;
                } else {
                    resp->version = PROTO_HTTP_UNKNOWN;
                    return WBT_ERROR;
                }

                resp->recv.offset += ver.len - 1;
                resp->state = 1;
                resp->status = 0;
                break;
            }
            case 1: { // status
                if (ch == ' ') {
                    resp->state = 2;
                    break;
                }

                if (ch < '0' || ch > '9') {
                    return WBT_ERROR;
                }

                resp->status = resp->status * 10 + (ch - '0');
                break;
            }
            case 2: {
                if (ch == '\r') {
                    resp->state = 4;
                }
                break;
            }
            case 4: {
                if (ch == '\n') {
                    resp->state = 5;
                    resp->headers.start = resp->recv.offset + 1;
                    resp->header_key.start = resp->recv.offset + 1;
                } else {
                    return WBT_ERROR;
                }
                break;
            }
            case 5: {
                if (ch == ':') {
                    resp->header_key.len = resp->recv.offset - resp->header_key.start;
                    resp->state = 6;
                }
                break;
            }
            case 6: {
                if (ch != ' ') {
                    resp->header_value.start = resp->recv.offset;
                    resp->state = 7;
                }
                break;
            }
            case 7: {
                if (ch == '\r') {
                    resp->header_value.len = resp->recv.offset - resp->header_value.start;
                    wbt_str_t header_key, header_value;
                    wbt_offset_to_str(resp->header_key, header_key, resp->recv.buf);
                    wbt_offset_to_str(resp->header_value, header_value, resp->recv.buf);

                    wbt_debug("%.*s", header_key.len, header_key.str);
                    
                    int i;
                    for( i = 1 ; i < HEADER_LENGTH ; i ++ ) {
                        if( wbt_strnicmp( &header_key, &HTTP_HEADERS[i], HTTP_HEADERS[i].len ) == 0 ) {
                            switch (i) {
                                case HEADER_CONNECTION: {
                                    wbt_str_t keep_alive = wbt_string("keep-alive");
                                    if( wbt_strnicmp( &header_value, &keep_alive, keep_alive.len ) == 0 ) {
                                        /* 声明为 keep-alive 连接 */
                                        resp->keep_alive = 1;
                                    } else {
                                        resp->keep_alive = 0;
                                    }
                                    break;
                                }
                                case HEADER_CONTENT_LENGTH: {
                                    resp->content_length = wbt_atoi(&header_value);
                                    break;
                                }
                                case HEADER_TRANSFER_ENCODING: {
                                    wbt_str_t chunked = wbt_string("chunked");
                                    if( wbt_strnicmp( &header_value, &chunked, chunked.len ) == 0 ) {
                                        resp->chunked = 1;
                                    } else {
                                        resp->chunked = 0;
                                    }
                                }
                            }
                        }
                    }

                    resp->state = 8;
                }
                break;
            }
            case 8: {
                if (ch == '\n') {
                    resp->state = 9;
                } else {
                    return WBT_ERROR;
                }
                break;
            }
            case 9: {
                if (ch == '\r') {
                    resp->state = 10;
                } else {
                    resp->header_key.start = resp->recv.offset;
                    resp->state = 5;
                }
                break;
            }
            case 10: {
                if (ch == '\n') {
                    resp->headers.len = resp->recv.offset - resp->headers.start - 3;
                    resp->recv.offset ++;
                    resp->body.start = resp->recv.offset;
                    resp->state = 11;
                    return WBT_OK;
                } else {
                    return WBT_ERROR;
                }
                break;
            }
            case 11:
                return WBT_OK;
            default:
                return WBT_ERROR;
        }

        resp->recv.offset ++;
    }

    return WBT_AGAIN; // 数据不完整
}

wbt_status wbt_http_parse_response_body(wbt_http_upsteam_response_t *resp) {
    if (resp->content_length >= 0) {
        if (resp->recv.length >= resp->recv.offset + resp->content_length) {
            resp->body.len = resp->content_length;
            resp->recv.offset += resp->content_length;
            return WBT_OK;
        } else {
            return WBT_AGAIN;
        }
    } else if (resp->chunked) {
        // TODO
        while (1) {
            break;
        }
        return WBT_OK;
    } else {
        return WBT_AGAIN;
    }
}

wbt_status wbt_http_parse_response(wbt_http_upsteam_response_t *resp) {
    if (wbt_http_parse_response_header(resp) != WBT_OK) {
        return WBT_ERROR;
    }

    return wbt_http_parse_response_body(resp);
}