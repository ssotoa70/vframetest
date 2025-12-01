/*
 * http_server.h
 *
 * Phase 4B: REST API HTTP Server
 * Core server definitions and API for vframetest REST interface
 *
 * Features:
 * - Socket-based HTTP server (no external dependencies)
 * - Request parsing and routing
 * - Response building and formatting
 * - Thread-safe operation
 * - Error handling
 */

#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>

/* Server configuration */
#define HTTP_SERVER_DEFAULT_PORT 8888
#define HTTP_SERVER_DEFAULT_BACKLOG 10
#define HTTP_SERVER_MAX_CONNECTIONS 100
#define HTTP_SERVER_REQUEST_BUFFER_SIZE 8192
#define HTTP_SERVER_RESPONSE_BUFFER_SIZE 65536
#define HTTP_SERVER_MAX_HEADERS 32
#define HTTP_SERVER_MAX_URL_LENGTH 2048

/* HTTP Methods */
typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_HEAD,
    HTTP_OPTIONS,
    HTTP_INVALID
} http_method_t;

/* HTTP Status Codes */
typedef enum {
    HTTP_200_OK = 200,
    HTTP_201_CREATED = 201,
    HTTP_204_NO_CONTENT = 204,
    HTTP_400_BAD_REQUEST = 400,
    HTTP_401_UNAUTHORIZED = 401,
    HTTP_403_FORBIDDEN = 403,
    HTTP_404_NOT_FOUND = 404,
    HTTP_405_METHOD_NOT_ALLOWED = 405,
    HTTP_409_CONFLICT = 409,
    HTTP_500_INTERNAL_ERROR = 500,
    HTTP_501_NOT_IMPLEMENTED = 501,
    HTTP_503_SERVICE_UNAVAILABLE = 503
} http_status_t;

/* HTTP Header */
typedef struct {
    char *name;
    char *value;
} http_header_t;

/* HTTP Request */
typedef struct {
    http_method_t method;
    char *url;
    char *query_string;
    char *body;
    size_t body_length;
    http_header_t *headers;
    size_t header_count;
    char *host;
    char *content_type;
} http_request_t;

/* HTTP Response */
typedef struct {
    http_status_t status;
    char *status_message;
    http_header_t *headers;
    size_t header_count;
    char *body;
    size_t body_length;
    time_t generated_at;
} http_response_t;

/* Request Handler Function Type */
typedef http_response_t* (*http_handler_t)(const http_request_t *req);

/* Route Definition */
typedef struct {
    http_method_t method;
    const char *path_pattern;
    http_handler_t handler;
    const char *description;
} http_route_t;

/* Server Context */
typedef struct {
    int listen_socket;
    int port;
    int max_connections;
    int active_connections;
    int running;

    /* Request/Response buffers */
    char *request_buffer;
    char *response_buffer;

    /* Routes */
    http_route_t *routes;
    size_t route_count;

    /* Statistics */
    uint64_t total_requests;
    uint64_t total_responses;
    uint64_t total_errors;
    uint64_t bytes_received;
    uint64_t bytes_sent;
} http_server_t;

/* ============================================================================
 * HTTP Server Lifecycle Functions
 * ============================================================================ */

/**
 * Initialize HTTP server
 * Returns allocated server context or NULL on error
 */
http_server_t* http_server_create(int port);

/**
 * Start listening for connections
 * Returns 0 on success, -1 on error
 */
int http_server_start(http_server_t *server);

/**
 * Stop server and free resources
 */
void http_server_stop(http_server_t *server);

/**
 * Destroy server context and free all memory
 */
void http_server_destroy(http_server_t *server);

/* ============================================================================
 * HTTP Request/Response Functions
 * ============================================================================ */

/**
 * Parse HTTP request from buffer
 * Returns allocated request or NULL on error
 */
http_request_t* http_request_parse(const char *buffer, size_t buffer_len);

/**
 * Get HTTP method name
 */
const char* http_method_to_string(http_method_t method);

/**
 * Parse HTTP method from string
 */
http_method_t http_method_from_string(const char *method_str);

/**
 * Create HTTP response
 * Returns allocated response or NULL on error
 */
http_response_t* http_response_create(http_status_t status);

/**
 * Add header to response
 */
int http_response_add_header(http_response_t *response,
                             const char *name,
                             const char *value);

/**
 * Set response body
 */
int http_response_set_body(http_response_t *response,
                          const char *body,
                          size_t length);

/**
 * Convert response to HTTP message
 * Returns dynamically allocated string (caller must free)
 */
char* http_response_to_string(const http_response_t *response,
                             size_t *length);

/**
 * Free request structure
 */
void http_request_free(http_request_t *request);

/**
 * Free response structure
 */
void http_response_free(http_response_t *response);

/* ============================================================================
 * HTTP Routing Functions
 * ============================================================================ */

/**
 * Register a route with handler
 * Returns 0 on success, -1 on error
 */
int http_server_register_route(http_server_t *server,
                               http_method_t method,
                               const char *path_pattern,
                               http_handler_t handler);

/**
 * Handle incoming request and return response
 * Caller is responsible for freeing response
 */
http_response_t* http_server_handle_request(http_server_t *server,
                                           const http_request_t *request);

/**
 * Find matching route for request
 */
http_route_t* http_server_find_route(http_server_t *server,
                                    const http_request_t *request);

/* ============================================================================
 * HTTP Utility Functions
 * ============================================================================ */

/**
 * Get HTTP status message
 */
const char* http_status_message(http_status_t status);

/**
 * Create JSON error response
 */
char* http_error_json(http_status_t status, const char *message);

/**
 * URL decode string (in-place)
 */
void http_url_decode(char *url);

/**
 * Parse query string into key-value pairs
 */
int http_parse_query_string(const char *query,
                           char **keys,
                           char **values,
                           size_t max_pairs);

/* ============================================================================
 * Server Statistics & Monitoring
 * ============================================================================ */

/**
 * Get server statistics
 */
typedef struct {
    uint64_t total_requests;
    uint64_t total_responses;
    uint64_t total_errors;
    uint64_t bytes_received;
    uint64_t bytes_sent;
    int active_connections;
    time_t uptime_seconds;
} http_server_stats_t;

http_server_stats_t http_server_get_stats(const http_server_t *server);

#endif /* HTTP_SERVER_H */
