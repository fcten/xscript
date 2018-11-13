#include "../common/str.h"
#include "../common/fun.h"
#include "../common/obj.h"
#include "std_net_http_server.h"

typedef enum { 
    METHOD_UNKNOWN,
    METHOD_GET,
    METHOD_POST,
    METHOD_HEAD,
    METHOD_PUT, 
    METHOD_DELETE, 
    METHOD_TRACE,
    METHOD_CONNECT,
    METHOD_OPTIONS,
    METHOD_LENGTH
} lgx_http_method_t;

typedef enum {
    HEADER_UNKNOWN,
    HEADER_CACHE_CONTROL,
    HEADER_CONNECTION,
    HEADER_DATE,
    HEADER_PRAGMA,
    HEADER_TRAILER,
    HEADER_TRANSFER_ENCODING,
    HEADER_UPGRADE,
    HEADER_VIA,
    HEADER_WARNING,
    HEADER_ACCEPT,
    HEADER_ACCEPT_CHARSET,
    HEADER_ACCEPT_ENCODING,
    HEADER_ACCEPT_LANGUAGE,
    HEADER_AUTHORIZATION,
    HEADER_EXPECT,
    HEADER_FROM,
    HEADER_HOST,
    HEADER_IF_MATCH,
    HEADER_IF_MODIFIED_SINCE,
    HEADER_IF_NONE_MATCH,
    HEADER_IF_RANGE,
    HEADER_IF_UNMODIFIED_SINCE,
    HEADER_MAX_FORWARDS,
    HEADER_PROXY_AUTHORIZATION,
    HEADER_RANGE,
    HEADER_REFERER,
    HEADER_TE,
    HEADER_USER_AGENT,
    HEADER_ACCEPT_RANGES,
    HEADER_AGE,
    HEADER_ETAG,
    HEADER_LOCATION,
    HEADER_PROXY_AUTENTICATE,
    HEADER_RETRY_AFTER,
    HEADER_SERVER,
    HEADER_VARY,
    HEADER_WWW_AUTHENTICATE,
    HEADER_ALLOW,
    HEADER_CONTENT_ENCODING,
    HEADER_CONTENT_LANGUAGE,
    HEADER_CONTENT_LENGTH,
    HEADER_CONTENT_LOCATION,
    HEADER_CONTENT_MD5,
    HEADER_CONTENT_RANGE,
    HEADER_CONTENT_TYPE,
    HEADER_EXPIRES,
    HEADER_LAST_MODIFIED,
    HEADER_COOKIE,
    HEADER_SET_COOKIE,
    HEADER_ACCESS_CONTROL_ALLOW_ORIGIN,
#ifdef WITH_WEBSOCKET
    HEADER_SEC_WEBSOCKET_KEY,
    HEADER_SEC_WEBSOCKET_VERSION,
    HEADER_SEC_WEBSOCKET_PROTOCOL,
    HEADER_SEC_WEBSOCKET_EXTENSIONS,
    HEADER_SEC_WEBSOCKET_ACCEPT,
#endif
    HEADER_LENGTH
} lgx_http_line_t;

typedef enum {
    STATUS_UNKNOWN,  // Unknown
    STATUS_100,  // Continue
    STATUS_101,  // Switching Protocols
    STATUS_102,  // Processing
    STATUS_200,  // OK
    STATUS_201,  // Created
    STATUS_202,  // Accepted
    STATUS_203,  // Non_Authoritative Information
    STATUS_204,  // No Content
    STATUS_205,  // Reset Content
    STATUS_206,  // Partial Content
    STATUS_207,  // Multi-Status
    STATUS_300,  // Multiple Choices
    STATUS_301,  // Moved Permanently
    STATUS_302,  // Found
                 // Moved Temporarily
    STATUS_303,  // See Other
    STATUS_304,  // Not Modified
    STATUS_305,  // Use Proxy
    STATUS_307,  // Temporary Redirect
    STATUS_400,  // Bad Request
    STATUS_401,  // Unauthorized
    STATUS_402,  // Payment Required
    STATUS_403,  // Forbidden
    STATUS_404,  // Not Found
    STATUS_405,  // Method Not Allowed
    STATUS_406,  // Not Acceptable
    STATUS_407,  // Proxy Authentication Required
    STATUS_408,  // Request Time_out
    STATUS_409,  // Conflict
    STATUS_410,  // Gone
    STATUS_411,  // Length Required
    STATUS_412,  // Precondition Failed
    STATUS_413,  // Request Entity Too Large
    STATUS_414,  // Request_URI Too Large
    STATUS_415,  // Unsupported Media Type
    STATUS_416,  // Requested range not satisfiable
    STATUS_417,  // Expectation Failed
    STATUS_422,  // Unprocessable Entity
    STATUS_423,  // Locked
    STATUS_424,  // Failed Dependency
    STATUS_426,  // Upgrade Required
    STATUS_500,  // Internal Server Error
    STATUS_501,  // Not Implemented
    STATUS_502,  // Bad Gateway
    STATUS_503,  // Service Unavailable
    STATUS_504,  // Gateway Time_out
    STATUS_505,  // HTTP Version not supported
    STATUS_506,  // Variant Also Negotiates
    STATUS_507,  // Insufficient Storage
    STATUS_510,  // Not Extended
    STATUS_LENGTH
} lgx_http_status_t;

static wbt_str_t REQUEST_METHOD[] = {
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

static wbt_str_t HTTP_HEADERS[] = {
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
    //wbt_string("Moved Temporarily"), // 注意：这并不是一个标准的 HTTP/1.1 状态码，只是为了兼容而添加
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

typedef struct {
    // 状态机状态
    unsigned state;
    // 接收缓冲区
    struct {
        char *buf;
        unsigned length;
        unsigned offset;
    } recv;  
    // 解析好的请求
    struct {
        lgx_http_method_t method;
        wbt_str_offset_t uri;
        wbt_str_offset_t params;
        wbt_str_offset_t version;
        lgx_hash_t *headers;
        wbt_str_offset_t body;
    } req;
} lgx_http_t;

static wbt_status std_net_http_parse_body(lgx_http_t *http) {
    return WBT_OK;
}

static wbt_status std_net_http_parse_header(lgx_http_t *http) {
    while (http->recv.offset < http->recv.length) {
        char ch = http->recv.buf[http->recv.offset];

        switch (http->state) {
            case 0: { // 解析 method
                if (ch == ' ') {
                    wbt_str_t method;
                    method.str = http->recv.buf;
                    method.len = http->recv.offset;
                    
                    http->req.method = METHOD_UNKNOWN;
                    int i;
                    for(i = METHOD_UNKNOWN + 1 ; i < METHOD_LENGTH ; i++ ) {
                        if( wbt_strncmp(&method,
                                &REQUEST_METHOD[i],
                                REQUEST_METHOD[i].len ) == 0 ) {
                            http->req.method = i;
                            break;
                        }
                    }
                    
                    if (http->req.method == METHOD_UNKNOWN) {
                        return WBT_ERROR;
                    } else {
                        http->state = 1;
                        http->req.uri.start = http->recv.offset + 1;
                    }
                } else if (http->recv.offset >= 8) { // 长度限制
                    return WBT_ERROR;
                }
                break;
            }
            case 1: { // 解析 uri
                if (ch == ' ') {
                    http->req.uri.len = http->recv.offset - http->req.uri.start;
                    http->state = 3;
                    http->req.params.start = 0;
                    http->req.params.len = 0;
                    http->req.version.start = http->recv.offset + 1;
                } else if (ch == '?') {
                    http->req.uri.len = http->recv.offset - http->req.uri.start;
                    http->state = 2;
                    http->req.params.start = http->recv.offset + 1;
                }
                break;
            }
            case 2: { // 解析 param
                if (ch == ' ') {
                    http->req.params.len = http->recv.offset - http->req.params.start;
                    http->state = 3;
                    http->req.version.start = http->recv.offset + 1;
                }
                break;
            }
            case 3: { // 解析 http 版本号
                if (ch == '\r') {
                    http->req.version.len = http->recv.offset - http->req.version.start;
                    http->state = 4;
                }
                break;
            }
            case 4: {
                if (ch == '\n') {
                    http->state = 5;
                } else {
                    return WBT_ERROR;
                }
                break;
            }
            case 5: {
                if (ch == '\r') {
                    http->state = 6;
                } else {
                    http->state = 8;
                }
                break;
            }
            case 6: {
                if (ch == '\n') {
                    http->state = 7;
                } else {
                    return WBT_ERROR;
                }
                break;
            }
            case 7: { // 解析 body
                return WBT_OK;
            }
            case 8: { // 解析 header
                if (ch == '\r') {
                    http->state = 4;
                }
                break;
            }
            default:
                return WBT_ERROR;
        }

        http->recv.offset ++;
    }

    return WBT_AGAIN; // 数据不完整
}

static int std_net_http_server_load_symbols(lgx_hash_t *hash) {
    lgx_val_t *parent = lgx_ext_get_symbol(hash, "Server");
    if (!parent || parent->type != T_OBJECT) {
        return 1;
    }

    lgx_str_t name;
    name.buffer = "HttpServer";
    name.length = sizeof("HttpServer")-1;

    lgx_val_t symbol;
    symbol.type = T_OBJECT;
    symbol.v.obj = lgx_obj_create(&name);
    symbol.v.obj->parent = parent->v.obj;

    if (lgx_ext_add_symbol(hash, symbol.v.obj->name->buffer, &symbol)) {
        return 1;
    }

    return 0;
}

lgx_buildin_ext_t ext_std_net_http_server_ctx = {
    "std.net.http",
    std_net_http_server_load_symbols
};