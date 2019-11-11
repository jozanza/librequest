#pragma once

#include <stdbool.h>

#define API_RESPONSE_MAX_BUFFER_SIZE (1024 * 10)
#define API_REQUEST_MAX_BUFFER_SIZE (1024 * 10)
#define API_REQUEST_MAX_HOSTNAME_LEN 1024
#define API_REQUEST_MAX_PATHNAME_LEN 1024
#define API_REQUEST_MAX_HEADERS_LEN 1024
#define API_REQUEST_MAX_BODY_LEN 4096

// Enumeration of HTTP methods
typedef enum RequestMethod {
  RequestMethod_GET,
  RequestMethod_DELETE,
  RequestMethod_PATCH,
  RequestMethod_POST,
  RequestMethod_PUT,
} RequestMethod;

// HTTP request options
typedef struct RequestOptions {
  RequestMethod method;
  char* pathname;
  char* hostname;
  char* headers; // NOTE: each header must end in \r\n as per the HTTTP spec
  int port;
  char* body;
  bool debug;
} RequestOptions;

// Sends a network request and returns the response body 🙌
// Note: the caller is in charge of freeing the return value
char* Request(RequestOptions* opts);

// A callback that is fired once the async request completes
typedef void (*RequestCompleteCallback)(char* responseBody);

// Sends a network request on its own thread 🙌 and triggers a callback with the response body
// Returns -1 if request thread fails to start
int RequestAsync(RequestOptions* opts, RequestCompleteCallback onComplete);
