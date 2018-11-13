#ifndef __WBT_HTTP_H__
#define	__WBT_HTTP_H__

#ifdef	__cplusplus
extern "C" {
#endif

#include "../webit.h"
#include "../common/wbt_string.h"

#define WITH_WEBSOCKET

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
    PROTO_HTTP_UNKNOWN,
    PROTO_HTTP_1_0,
    PROTO_HTTP_1_1
} lgx_http_version_t;

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

typedef struct {
    // 状态机状态
    unsigned state;
    // 接收缓冲区
    struct {
        char *buf;
        unsigned length;
        unsigned offset;
    } recv;  
    // 保存解析好的请求
    lgx_http_method_t method;
    wbt_str_offset_t uri;
    wbt_str_offset_t params;
    lgx_http_version_t version;
    wbt_str_offset_t headers;
    wbt_str_offset_t body;

    wbt_str_offset_t header_key;
    wbt_str_offset_t header_value;
    unsigned keep_alive;
    int content_length;
} lgx_http_request_t;

const char *wbt_http_method(lgx_http_method_t method);

wbt_status wbt_http_parse(lgx_http_request_t *req);
wbt_status wbt_http_parse_header(lgx_http_request_t *req);
wbt_status wbt_http_parse_body(lgx_http_request_t *req);

#ifdef	__cplusplus
}
#endif

#endif	/* __WBT_HTTP_H__ */