#pragma once

#include <stdbool.h>

#define HTTP_RESPONSE_MAX_BUFFER_SIZE (1024 * 10)
#define HTTP_REQUEST_MAX_BUFFER_SIZE (1024 * 10)
#define HTTP_REQUEST_MAX_HOSTNAME_LEN 1024
#define HTTP_REQUEST_MAX_PATHNAME_LEN 1024
#define HTTP_REQUEST_MAX_HEADERS_LEN 1024
#define HTTP_REQUEST_MAX_BODY_LEN 4096

// Enumeration of HTTP methods
typedef enum HTTPRequestMethod {
  HTTPRequestMethod_GET,
  HTTPRequestMethod_DELETE,
  HTTPRequestMethod_PATCH,
  HTTPRequestMethod_POST,
  HTTPRequestMethod_PUT,
} HTTPRequestMethod;

// Some common HTTP request port numbers
typedef enum HTTPRequestPort {
  HTTPRequestPort_DEFAULT_HTTP  = 80,
  HTTPRequestPort_DEFAULT_HTTPS = 443,
} HTTPRequestPort;

// HTTP request options
typedef struct HTTPRequest {
  HTTPRequestMethod method;
  char* pathname;
  char* hostname;
  char* headers; // NOTE: each header must end in \r\n as per the HTTTP spec
  int port;
  char* body;
  bool debug;
} HTTPRequest;

// HTTP response
typedef struct HTTPResponse {
  int status;
  char* headers;
  char* body;
} HTTPResponse;

// Sends a network request and returns the response 🙌
// The caller is in charge of freeing the response with FreeResponse()
HTTPResponse* Request(HTTPRequest* req);

// Frees an HTTPResponse struct and all of its fields
void FreeResponse(HTTPResponse*);

// A callback that is fired once the async request completes
typedef void (*HTTPRequestCallback)(HTTPResponse* res);

// Sends a network request on its own thread 🙌 and triggers a callback with the response body
// Returns -1 if request thread fails to start
int RequestAsync(HTTPRequest*, HTTPRequestCallback);
