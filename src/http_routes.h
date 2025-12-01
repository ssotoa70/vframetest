/*
 * http_routes.h
 *
 * Phase 4B: HTTP Routes and Endpoint Handlers
 * Route definitions and handler declarations
 */

#ifndef HTTP_ROUTES_H
#define HTTP_ROUTES_H

#include "http_server.h"

/* ============================================================================
 * Route Handler Declarations
 * ============================================================================ */

/* Health Check and Status */
http_response_t* handle_health_check(const http_request_t *req);
http_response_t* handle_api_status(const http_request_t *req);
http_response_t* handle_api_progress(const http_request_t *req);

/* Metrics */
http_response_t* handle_api_metrics(const http_request_t *req);
http_response_t* handle_api_io_modes(const http_request_t *req);
http_response_t* handle_api_filesystem(const http_request_t *req);

/* History and Summary */
http_response_t* handle_api_history(const http_request_t *req);
http_response_t* handle_api_summary(const http_request_t *req);

/* Control Operations */
http_response_t* handle_api_pause(const http_request_t *req);
http_response_t* handle_api_resume(const http_request_t *req);
http_response_t* handle_api_stop(const http_request_t *req);

/* Export Operations */
http_response_t* handle_api_export_json(const http_request_t *req);
http_response_t* handle_api_export_csv(const http_request_t *req);

/* ============================================================================
 * Route Registration
 * ============================================================================ */

/**
 * Register all API routes with server
 * Returns 0 on success, -1 on error
 */
int http_routes_register_all(http_server_t *server);

#endif /* HTTP_ROUTES_H */
