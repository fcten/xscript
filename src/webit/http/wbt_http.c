#include "../common/wbt_error.h"
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
    wbt_string("100 Continue"),
    wbt_string("101 Switching Protocols"), 
    wbt_string("102 Processing"),
    wbt_string("200 OK"),
    wbt_string("201 Created"),
    wbt_string("202 Accepted"),
    wbt_string("203 Non-Authoritative Information"),
    wbt_string("204 No Content"),
    wbt_string("205 Reset Content"),
    wbt_string("206 Partial Content"),
    wbt_string("207 Multi-Status"),
    wbt_string("300 Multiple Choices"),
    wbt_string("301 Moved Permanently"),
    wbt_string("302 Found"),
    wbt_string("302 Moved Temporarily"), // 注意：这并不是一个标准的 HTTP/1.1 状态码，只是为了兼容而添加
    wbt_string("303 See Other"),
    wbt_string("304 Not Modified"),
    wbt_string("305 Use Proxy"),
    wbt_string("307 Temporary Redirect"),
    wbt_string("400 Bad Request"),
    wbt_string("401 Unauthorized"),
    wbt_string("402 Payment Required"),
    wbt_string("403 Forbidden"),
    wbt_string("404 Not Found"),
    wbt_string("405 Method Not Allowed"),
    wbt_string("406 Not Acceptable"),
    wbt_string("407 Proxy Authentication Required"),
    wbt_string("408 Request Time-out"),
    wbt_string("409 Conflict"),
    wbt_string("410 Gone"),
    wbt_string("411 Length Required"),
    wbt_string("412 Precondition Failed"),
    wbt_string("413 Request Entity Too Large"),
    wbt_string("414 Request-URI Too Large"),
    wbt_string("415 Unsupported Media Type"),
    wbt_string("416 Requested range not satisfiable"),
    wbt_string("417 Expectation Failed"),
    wbt_string("422 Unprocessable Entity"),
    wbt_string("423 Locked"),
    wbt_string("424 Failed Dependency"),
    wbt_string("426 Upgrade Required"),
    wbt_string("500 Internal Server Error"),
    wbt_string("501 Not Implemented"),
    wbt_string("502 Bad Gateway"),
    wbt_string("503 Service Unavailable"),
    wbt_string("504 Gateway Time-out"),
    wbt_string("505 HTTP Version not supported"),
    wbt_string("506 Variant Also Negotiates"),
    wbt_string("507 Insufficient Storage"),
    wbt_string("510 Not Extended"),
    wbt_null_string
};

const char *wbt_http_method(lgx_http_method_t method) {
    if (method > 0 && method < METHOD_LENGTH ) {
        return REQUEST_METHOD[method].str;
    }
    return REQUEST_METHOD[0].str;
}

wbt_status wbt_http_parse(lgx_http_request_t *req) {
    wbt_status ret = wbt_http_parse_header(req);
    if (ret != WBT_OK) {
        return ret;
    }

    return wbt_http_parse_body(req);
}

wbt_status wbt_http_parse_body(lgx_http_request_t *req) {
    req->body.start = req->recv.offset;

    if (req->content_length > 0) {
        if (req->recv.length >= req->recv.offset + req->content_length) {
            req->body.len = req->content_length;
            return WBT_OK;
        } else {
            return WBT_AGAIN;
        }
    } else {
        return WBT_OK;
    }
}

wbt_status wbt_http_parse_header(lgx_http_request_t *req) {
    while (req->recv.offset < req->recv.length) {
        char ch = req->recv.buf[req->recv.offset];

        switch (req->state) {
            case 0: { // 解析 method
                if (ch == ' ') {
                    wbt_str_t method;
                    method.str = req->recv.buf;
                    method.len = req->recv.offset;

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
                        req->state = 1;
                        req->uri.start = req->recv.offset + 1;
                    }
                } else if (req->recv.offset >= 8) { // 长度限制
                    return WBT_ERROR;
                }
                break;
            }
            case 1: { // 解析 uri
                if (ch == ' ') {
                    req->uri.len = req->recv.offset - req->uri.start;
                    req->state = 3;
                    req->params.start = 0;
                    req->params.len = 0;
                } else if (ch == '?') {
                    req->uri.len = req->recv.offset - req->uri.start;
                    req->state = 2;
                    req->params.start = req->recv.offset + 1;
                }
                break;
            }
            case 2: { // 解析 param
                if (ch == ' ') {
                    req->params.len = req->recv.offset - req->params.start;
                    req->state = 3;
                }
                break;
            }
            case 3: { // 解析 http 版本号
                if (req->recv.length < req->recv.offset + 10) {
                    return WBT_AGAIN;
                }

                wbt_str_t ver;
                wbt_str_t http_ver_1_0 = wbt_string("HTTP/1.0\r\n");
                wbt_str_t http_ver_1_1 = wbt_string("HTTP/1.1\r\n");

                ver.str = req->recv.buf + req->recv.offset;
                ver.len = http_ver_1_0.len;

                if (wbt_strcmp(&ver, &http_ver_1_0) == 0) {
                    req->version = PROTO_HTTP_1_0;
                } else if (wbt_strcmp(&ver, &http_ver_1_1) == 0) {
                    req->version = PROTO_HTTP_1_1;
                } else {
                    req->version = PROTO_HTTP_UNKNOWN;
                    return WBT_ERROR;
                }

                req->recv.offset += ver.len - 1;
                req->state = 4;
                req->headers.start = req->recv.offset + 1;
                req->header_key.start = req->recv.offset + 1;
                break;
            }
            case 4: {
                if (ch == ':') {
                    req->header_key.len = req->recv.offset - req->header_key.start;
                    req->state = 5;
                }
                break;
            }
            case 5: {
                if (ch != ' ') {
                    req->header_value.start = req->recv.offset;
                    req->state = 6;
                }
            }
            case 6: {
                if (ch == '\r') {
                    req->header_value.len = req->recv.offset - req->header_value.start;
                    wbt_str_t header_key, header_value;
                    wbt_offset_to_str(req->header_key, header_key, req->recv.buf);
                    wbt_offset_to_str(req->header_value, header_value, req->recv.buf);

                    wbt_debug("%.*s", header_key.len, header_key.str);
                    
                    int i;
                    for( i = 1 ; i < HEADER_LENGTH ; i ++ ) {
                        if( wbt_strnicmp( &header_key, &HTTP_HEADERS[i], HTTP_HEADERS[i].len ) == 0 ) {
                            switch (i) {
                                case HEADER_CONNECTION: {
                                    wbt_str_t keep_alive = wbt_string("keep-alive");
                                    if( wbt_strnicmp( &header_value, &keep_alive, keep_alive.len ) == 0 ) {
                                        /* 声明为 keep-alive 连接 */
                                        req->keep_alive = 1;
                                    } else {
                                        req->keep_alive = 0;
                                    }
                                    break;
                                }
                                case HEADER_CONTENT_LENGTH: {
                                    req->content_length = wbt_atoi(&header_value);
                                    break;
                                }
                            }
                        }
                    }

                    req->state = 7;
                }
                break;
            }
            case 7: {
                if (ch == '\n') {
                    req->state = 8;
                } else {
                    return WBT_ERROR;
                }
                break;
            }
            case 8: {
                if (ch == '\r') {
                    req->state = 9;
                } else {
                    req->header_key.start = req->recv.offset;
                    req->state = 4;
                }
                break;
            }
            case 9: {
                if (ch == '\n') {
                    req->headers.len = req->recv.offset - req->headers.start - 3;
                    req->recv.offset ++;

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
