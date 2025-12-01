/*
 * http_server.c
 *
 * Phase 4B: REST API HTTP Server Implementation
 * Socket-based HTTP server with request parsing and routing
 */

#include "http_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>

/* ============================================================================
 * Server Lifecycle Functions
 * ============================================================================ */

http_server_t* http_server_create(int port)
{
    http_server_t *server = malloc(sizeof(http_server_t));
    if (!server)
        return NULL;

    memset(server, 0, sizeof(http_server_t));

    server->port = port > 0 ? port : HTTP_SERVER_DEFAULT_PORT;
    server->listen_socket = -1;
    server->max_connections = HTTP_SERVER_MAX_CONNECTIONS;
    server->running = 0;

    /* Allocate buffers */
    server->request_buffer = malloc(HTTP_SERVER_REQUEST_BUFFER_SIZE);
    server->response_buffer = malloc(HTTP_SERVER_RESPONSE_BUFFER_SIZE);

    if (!server->request_buffer || !server->response_buffer) {
        free(server->request_buffer);
        free(server->response_buffer);
        free(server);
        return NULL;
    }

    /* Allocate routes array */
    server->routes = malloc(sizeof(http_route_t) * 32);
    if (!server->routes) {
        free(server->request_buffer);
        free(server->response_buffer);
        free(server);
        return NULL;
    }

    server->route_count = 0;
    return server;
}

int http_server_start(http_server_t *server)
{
    if (!server || server->listen_socket >= 0)
        return -1;

    struct sockaddr_in addr;
    int reuse = 1;

    /* Create socket */
    server->listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server->listen_socket < 0) {
        perror("socket");
        return -1;
    }

    /* Set socket options */
    if (setsockopt(server->listen_socket, SOL_SOCKET, SO_REUSEADDR,
                   &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        close(server->listen_socket);
        return -1;
    }

    /* Bind socket */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(server->port);

    if (bind(server->listen_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server->listen_socket);
        return -1;
    }

    /* Listen for connections */
    if (listen(server->listen_socket, HTTP_SERVER_DEFAULT_BACKLOG) < 0) {
        perror("listen");
        close(server->listen_socket);
        return -1;
    }

    server->running = 1;
    return 0;
}

void http_server_stop(http_server_t *server)
{
    if (!server)
        return;

    server->running = 0;

    if (server->listen_socket >= 0) {
        close(server->listen_socket);
        server->listen_socket = -1;
    }
}

void http_server_destroy(http_server_t *server)
{
    if (!server)
        return;

    http_server_stop(server);

    free(server->request_buffer);
    free(server->response_buffer);
    free(server->routes);
    free(server);
}

/* ============================================================================
 * HTTP Request Functions
 * ============================================================================ */

http_request_t* http_request_parse(const char *buffer, size_t buffer_len)
{
    if (!buffer || buffer_len == 0)
        return NULL;

    http_request_t *request = malloc(sizeof(http_request_t));
    if (!request)
        return NULL;

    memset(request, 0, sizeof(http_request_t));

    /* Find end of request line */
    char *line_end = strchr(buffer, '\n');
    if (!line_end) {
        free(request);
        return NULL;
    }

    size_t line_len = line_end - buffer;
    if (line_len > 2 && buffer[line_len - 1] == '\r')
        line_len--;

    /* Parse request line: METHOD URL HTTP/1.1 */
    char method_str[16] = {0};
    char url[HTTP_SERVER_MAX_URL_LENGTH] = {0};
    char version[16] = {0};

    if (sscanf(buffer, "%15s %2047s %15s", method_str, url, version) != 3) {
        free(request);
        return NULL;
    }

    /* Parse method */
    request->method = http_method_from_string(method_str);
    if (request->method == HTTP_INVALID) {
        free(request);
        return NULL;
    }

    /* Parse URL and query string */
    char *query_start = strchr(url, '?');
    if (query_start) {
        *query_start = '\0';
        request->url = malloc(strlen(url) + 1);
        request->query_string = malloc(strlen(query_start + 1) + 1);
        if (request->url && request->query_string) {
            strcpy(request->url, url);
            strcpy(request->query_string, query_start + 1);
        }
        *query_start = '?'; /* Restore for safety */
    } else {
        request->url = malloc(strlen(url) + 1);
        if (request->url)
            strcpy(request->url, url);
    }

    /* Allocate headers array */
    request->headers = malloc(sizeof(http_header_t) * HTTP_SERVER_MAX_HEADERS);
    if (!request->headers) {
        http_request_free(request);
        return NULL;
    }

    /* Parse headers */
    const char *header_start = line_end + 1;
    while (header_start < buffer + buffer_len) {
        const char *header_end = strchr(header_start, '\n');
        if (!header_end)
            break;

        size_t header_len = header_end - header_start;
        if (header_len < 3)
            break; /* Empty line marks end of headers */

        /* Find colon separator */
        const char *colon = strchr(header_start, ':');
        if (colon) {
            size_t name_len = colon - header_start;
            const char *value_start = colon + 1;
            while (*value_start == ' ' && value_start < header_end)
                value_start++;

            size_t value_len = header_end - value_start;
            if (value_len > 0 && *(header_end - 1) == '\r')
                value_len--;

            if (request->header_count < HTTP_SERVER_MAX_HEADERS) {
                http_header_t *h = &request->headers[request->header_count];
                h->name = malloc(name_len + 1);
                h->value = malloc(value_len + 1);

                if (h->name && h->value) {
                    strncpy(h->name, header_start, name_len);
                    h->name[name_len] = '\0';
                    strncpy(h->value, value_start, value_len);
                    h->value[value_len] = '\0';
                    request->header_count++;
                }
            }
        }

        header_start = header_end + 1;
    }

    return request;
}

const char* http_method_to_string(http_method_t method)
{
    switch (method) {
        case HTTP_GET: return "GET";
        case HTTP_POST: return "POST";
        case HTTP_PUT: return "PUT";
        case HTTP_DELETE: return "DELETE";
        case HTTP_HEAD: return "HEAD";
        case HTTP_OPTIONS: return "OPTIONS";
        default: return "INVALID";
    }
}

http_method_t http_method_from_string(const char *method_str)
{
    if (!method_str)
        return HTTP_INVALID;

    if (strcmp(method_str, "GET") == 0)
        return HTTP_GET;
    if (strcmp(method_str, "POST") == 0)
        return HTTP_POST;
    if (strcmp(method_str, "PUT") == 0)
        return HTTP_PUT;
    if (strcmp(method_str, "DELETE") == 0)
        return HTTP_DELETE;
    if (strcmp(method_str, "HEAD") == 0)
        return HTTP_HEAD;
    if (strcmp(method_str, "OPTIONS") == 0)
        return HTTP_OPTIONS;

    return HTTP_INVALID;
}

void http_request_free(http_request_t *request)
{
    if (!request)
        return;

    free(request->url);
    free(request->query_string);
    free(request->body);

    if (request->headers) {
        for (size_t i = 0; i < request->header_count; i++) {
            free(request->headers[i].name);
            free(request->headers[i].value);
        }
        free(request->headers);
    }

    free(request);
}

/* ============================================================================
 * HTTP Response Functions
 * ============================================================================ */

http_response_t* http_response_create(http_status_t status)
{
    http_response_t *response = malloc(sizeof(http_response_t));
    if (!response)
        return NULL;

    memset(response, 0, sizeof(http_response_t));
    response->status = status;
    response->status_message = (char*)http_status_message(status);
    response->generated_at = time(NULL);

    /* Allocate headers array */
    response->headers = malloc(sizeof(http_header_t) * HTTP_SERVER_MAX_HEADERS);
    if (!response->headers) {
        free(response);
        return NULL;
    }

    return response;
}

int http_response_add_header(http_response_t *response,
                            const char *name,
                            const char *value)
{
    if (!response || !name || !value)
        return -1;

    if (response->header_count >= HTTP_SERVER_MAX_HEADERS)
        return -1;

    http_header_t *h = &response->headers[response->header_count];
    h->name = malloc(strlen(name) + 1);
    h->value = malloc(strlen(value) + 1);

    if (!h->name || !h->value) {
        free(h->name);
        free(h->value);
        return -1;
    }

    strcpy(h->name, name);
    strcpy(h->value, value);
    response->header_count++;

    return 0;
}

int http_response_set_body(http_response_t *response,
                          const char *body,
                          size_t length)
{
    if (!response || !body || length == 0)
        return -1;

    response->body = malloc(length + 1);
    if (!response->body)
        return -1;

    memcpy(response->body, body, length);
    response->body[length] = '\0';
    response->body_length = length;

    return 0;
}

char* http_response_to_string(const http_response_t *response,
                             size_t *length)
{
    if (!response || !length)
        return NULL;

    char *buffer = malloc(HTTP_SERVER_RESPONSE_BUFFER_SIZE);
    if (!buffer)
        return NULL;

    int offset = 0;

    /* Status line */
    offset += snprintf(buffer + offset,
                      HTTP_SERVER_RESPONSE_BUFFER_SIZE - offset,
                      "HTTP/1.1 %d %s\r\n",
                      response->status,
                      response->status_message);

    /* Standard headers */
    offset += snprintf(buffer + offset,
                      HTTP_SERVER_RESPONSE_BUFFER_SIZE - offset,
                      "Server: vframetest/25.16.0\r\n");

    offset += snprintf(buffer + offset,
                      HTTP_SERVER_RESPONSE_BUFFER_SIZE - offset,
                      "Content-Type: %s\r\n",
                      "application/json");

    if (response->body_length > 0) {
        offset += snprintf(buffer + offset,
                          HTTP_SERVER_RESPONSE_BUFFER_SIZE - offset,
                          "Content-Length: %zu\r\n",
                          response->body_length);
    }

    /* Custom headers */
    for (size_t i = 0; i < response->header_count; i++) {
        offset += snprintf(buffer + offset,
                          HTTP_SERVER_RESPONSE_BUFFER_SIZE - offset,
                          "%s: %s\r\n",
                          response->headers[i].name,
                          response->headers[i].value);
    }

    /* End of headers */
    offset += snprintf(buffer + offset,
                      HTTP_SERVER_RESPONSE_BUFFER_SIZE - offset,
                      "\r\n");

    /* Body */
    if (response->body && response->body_length > 0) {
        if (offset + response->body_length < HTTP_SERVER_RESPONSE_BUFFER_SIZE) {
            memcpy(buffer + offset, response->body, response->body_length);
            offset += response->body_length;
        }
    }

    *length = offset;
    return buffer;
}

void http_response_free(http_response_t *response)
{
    if (!response)
        return;

    if (response->headers) {
        for (size_t i = 0; i < response->header_count; i++) {
            free(response->headers[i].name);
            free(response->headers[i].value);
        }
        free(response->headers);
    }

    free(response->body);
    free(response);
}

/* ============================================================================
 * HTTP Routing Functions
 * ============================================================================ */

int http_server_register_route(http_server_t *server,
                              http_method_t method,
                              const char *path_pattern,
                              http_handler_t handler)
{
    if (!server || !path_pattern || !handler)
        return -1;

    /* For now, simple implementation - will expand for pattern matching */
    if (server->route_count >= 32)
        return -1;

    http_route_t *route = &server->routes[server->route_count];
    route->method = method;
    route->path_pattern = path_pattern;
    route->handler = handler;

    server->route_count++;
    return 0;
}

http_route_t* http_server_find_route(http_server_t *server,
                                    const http_request_t *request)
{
    if (!server || !request || !request->url)
        return NULL;

    for (size_t i = 0; i < server->route_count; i++) {
        http_route_t *route = &server->routes[i];

        if (route->method != request->method)
            continue;

        /* Simple path matching - exact match for now */
        if (strcmp(route->path_pattern, request->url) == 0)
            return route;
    }

    return NULL;
}

http_response_t* http_server_handle_request(http_server_t *server,
                                           const http_request_t *request)
{
    if (!server || !request)
        return NULL;

    http_route_t *route = http_server_find_route(server, request);
    if (!route) {
        /* 404 Not Found */
        http_response_t *response = http_response_create(HTTP_404_NOT_FOUND);
        if (response) {
            char *error_body = http_error_json(HTTP_404_NOT_FOUND,
                                              "Path not found");
            if (error_body) {
                http_response_set_body(response, error_body, strlen(error_body));
                free(error_body);
            }
        }
        return response;
    }

    /* Call handler */
    return route->handler(request);
}

/* ============================================================================
 * HTTP Utility Functions
 * ============================================================================ */

const char* http_status_message(http_status_t status)
{
    switch (status) {
        case HTTP_200_OK: return "OK";
        case HTTP_201_CREATED: return "Created";
        case HTTP_204_NO_CONTENT: return "No Content";
        case HTTP_400_BAD_REQUEST: return "Bad Request";
        case HTTP_401_UNAUTHORIZED: return "Unauthorized";
        case HTTP_403_FORBIDDEN: return "Forbidden";
        case HTTP_404_NOT_FOUND: return "Not Found";
        case HTTP_405_METHOD_NOT_ALLOWED: return "Method Not Allowed";
        case HTTP_409_CONFLICT: return "Conflict";
        case HTTP_500_INTERNAL_ERROR: return "Internal Server Error";
        case HTTP_501_NOT_IMPLEMENTED: return "Not Implemented";
        case HTTP_503_SERVICE_UNAVAILABLE: return "Service Unavailable";
        default: return "Unknown";
    }
}

char* http_error_json(http_status_t status, const char *message)
{
    size_t size = 256;
    char *json = malloc(size);
    if (!json)
        return NULL;

    snprintf(json, size,
            "{\"error\":%d,\"message\":\"%s\"}",
            status,
            message ? message : "Unknown error");

    return json;
}

void http_url_decode(char *url)
{
    if (!url)
        return;

    char *src = url;
    char *dst = url;

    while (*src) {
        if (*src == '%' && src[1] && src[2]) {
            int val = 0;
            sscanf(src + 1, "%2x", &val);
            *dst = (char)val;
            src += 3;
        } else if (*src == '+') {
            *dst = ' ';
            src++;
        } else {
            *dst = *src;
            src++;
        }
        dst++;
    }
    *dst = '\0';
}

/* ============================================================================
 * Server Statistics
 * ============================================================================ */

http_server_stats_t http_server_get_stats(const http_server_t *server)
{
    http_server_stats_t stats;
    memset(&stats, 0, sizeof(stats));

    if (!server)
        return stats;

    stats.total_requests = server->total_requests;
    stats.total_responses = server->total_responses;
    stats.total_errors = server->total_errors;
    stats.bytes_received = server->bytes_received;
    stats.bytes_sent = server->bytes_sent;
    stats.active_connections = server->active_connections;

    return stats;
}
