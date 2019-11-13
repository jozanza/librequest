#include "request.h"

#include <netdb.h>       // hostent, gethostbyname, herror
#include <netinet/in.h>  // sockaddr_in, htons, IPPROTO_TCP
#include <netinet/tcp.h> // TCP_NODELAY
#include <stdio.h>       // perror
#include <stdlib.h>      // exit, malloc
#include <string.h>      // bcopy, bzero, strlen
#include <sys/socket.h>  // AF_INET, socket, setsockopt, connect, SHUT_RDWR, shutdown
#include <sys/types.h>   // int_port_t
#include <unistd.h>      // write, read, close

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <pthread.h>

//------------------------------------------------------------------------------
// Sockets
//------------------------------------------------------------------------------

static int SocketConnect(char* hostname, in_port_t port) {
  struct hostent* host;
  struct sockaddr_in addr;
  int on = 1;
  if ((host = gethostbyname(hostname)) == NULL) {
    herror("Couldn't get host info");
    return -1;
  }
  bcopy(host->h_addr, &addr.sin_addr, host->h_length);
  addr.sin_family = AF_INET;
  addr.sin_port   = htons(port);
  int sock        = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char*)&on, sizeof(int));
  if (sock == -1) {
    perror("Couldn't open socket");
    return -1;
  }
  if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("Couldn't connect to socket");
    return -1;
  }
  return sock;
}

//------------------------------------------------------------------------------
// SSL
//------------------------------------------------------------------------------

static pthread_mutex_t sslLock = PTHREAD_MUTEX_INITIALIZER;

static void initSSL(void) {
  pthread_mutex_lock(&sslLock);
  SSL_library_init();
  OpenSSL_add_all_algorithms(); /* Load cryptos, et.al. */
  SSL_load_error_strings();     /* Bring in and register error messages */
  pthread_mutex_unlock(&sslLock);
}

static SSL_CTX* InitSSLContext(void) {
  initSSL();
  const SSL_METHOD* method = SSLv23_client_method(); /* Create new client-method instance */
  SSL_CTX* ctx             = SSL_CTX_new(method);    /* Create new context */
  if (ctx == NULL) {
    ERR_print_errors_fp(stderr);
    return NULL;
  }
  return ctx;
}

//------------------------------------------------------------------------------
// Synchronous HTTP Requests
//------------------------------------------------------------------------------

// Creates the request headers and body to be written to the socket
static void serializeRequest(HTTPRequest* opts, char req[HTTP_REQUEST_MAX_BUFFER_SIZE + 1]) {
  char* version = "1.0";
  char* methodName;
  switch (opts->method) {
  case HTTPRequestMethod_GET:
    methodName = "GET";
    break;
  case HTTPRequestMethod_DELETE:
    methodName = "DELETE";
    break;
  case HTTPRequestMethod_PATCH:
    methodName = "PATCH";
    break;
  case HTTPRequestMethod_POST:
    methodName = "POST";
    break;
  case HTTPRequestMethod_PUT:
    methodName = "PUT";
    break;
  }
  if (HTTPRequestMethod_GET == opts->method) {
    snprintf(
        req,
        HTTP_REQUEST_MAX_BUFFER_SIZE,
        "%s %s HTTP/%s\r\n"
        "Host: %s\r\n"
        "%s"
        "\r\n",
        methodName,
        opts->pathname,
        version,
        opts->hostname,
        opts->headers);
  } else {
    snprintf(
        req,
        HTTP_REQUEST_MAX_BUFFER_SIZE,
        "%s %s HTTP/%s\r\n"
        "Host: %s\r\n"
        "%s"
        "Content-Length: %d\r\n"
        "\r\n"
        "%s",
        methodName,
        opts->pathname,
        version,
        opts->hostname,
        opts->headers == NULL ? "" : opts->headers,
        (int)strlen(opts->body),
        opts->body);
  }
}

// Helper function to split a string
// Used for parsing out the response body from the response headers
static char* split(char* str, const char* delim) {
  char* p = strstr(str, delim);
  if (p == NULL)
    return NULL;            // delimiter not found
  *p = '\0';                // terminate string after head
  return p + strlen(delim); // return tail substring
}

// Returns a copy of the input string allocated on the heap
static char* copyString(char* a) {
  if (a == NULL)
    return NULL;
  int len = strlen(a) + 1;
  char* b = malloc(len);
  strlcpy(b, a, len);
  return b;
}

void FreeResponse(HTTPResponse* res) {
  if (res == NULL) {
    return;
  }
  if (res->headers != NULL)
    free(res->headers);
  if (res->body != NULL)
    free(res->body);
  free(res);
}

HTTPResponse* Request(HTTPRequest* req) {
  if (!req) {
    perror("No HTTPRequest provided to Request()");
    return NULL;
  }
  if (req->debug)
    printf("----------------\n");

  // Will be allocated and filled with response body
  HTTPResponse* res = NULL;

  // Create socket and init SSL
  int sock = SocketConnect(req->hostname, req->port);
  if (sock == -1) {
    goto done;
  }
  SSL_CTX* sslctx = InitSSLContext();
  SSL* ssl        = SSL_new(sslctx);
  if (!ssl) {
    goto done;
  }
  SSL_set_tlsext_host_name(ssl, req->hostname);
  SSL_set_fd(ssl, sock);
  if (SSL_connect(ssl) <= 0) {
    // Error occurred, log and close down ssl
    ERR_print_errors_fp(stderr);
    goto done;
  }

  // Send request
  char reqBuffer[HTTP_REQUEST_MAX_BUFFER_SIZE + 1];
  serializeRequest(req, reqBuffer);
  if (req->debug) {
    printf("Sending request:\n%s\n", reqBuffer);
    printf("...\n");
  }
  size_t reqLen        = strlen(reqBuffer);
  ssize_t bytesWritten = SSL_write(ssl, reqBuffer, reqLen);
  if (bytesWritten < reqLen) {
    perror("Couldn't write to socket\n");
    goto done;
  }
  if (req->debug) {
    printf("Request sent!\n");
    printf("...\n");
  }

  // Read response
  char resBuffer[HTTP_RESPONSE_MAX_BUFFER_SIZE + 1];
  bzero(resBuffer, HTTP_REQUEST_MAX_BUFFER_SIZE + 1);
  if (req->debug) {
    printf("Awaiting response...\n");
  }
  ssize_t bytesRead;
  while ((bytesRead = SSL_read(ssl, resBuffer, HTTP_REQUEST_MAX_BUFFER_SIZE)) != 0) {
    if (req->debug)
      printf("Read %lu bytes...", strlen(resBuffer));
  }

  // Split response body from response headers
  char* body    = split(resBuffer, "\r\n\r\n");
  char* headers = resBuffer;
  if (req->debug) {
    printf("\n...\n");
    printf("[HEADER]\n\n%s\n\n", headers);
    printf("[BODY]\n\n%s\n\n", body);
  }

  // Update the response
  res          = calloc(1, sizeof(*res));
  res->status  = atoi((char[]){headers[9], headers[10], headers[11], 0});
  res->headers = copyString(headers);
  res->body    = copyString(body);

done:
  // Destroy socket
  shutdown(sock, SHUT_RDWR);
  close(sock);

  // Destroy SSL
  SSL_CTX_free(sslctx);

  // Return response body
  return res;
}

//------------------------------------------------------------------------------
// Async HTTP Requests
//------------------------------------------------------------------------------

// This struct holds HTTP request options and an onComplete function pointer
typedef struct AsyncRequestContext {
  HTTPRequest* options;
  HTTPRequestCallback onComplete;
} AsyncRequestContext;

// The pthread that handles the asynchronous request
static void* requestAsync(void* any) {
  // Convert thread arg to AsyncRequestContext
  AsyncRequestContext* ctx = any;

  // Make Request
  HTTPResponse* res = Request(ctx->options);
  ctx->onComplete(res);

  // Free allocated memory
  if (ctx->options->headers != NULL)
    free(ctx->options->headers);
  if (ctx->options->hostname != NULL)
    free(ctx->options->hostname);
  if (ctx->options->pathname != NULL)
    free(ctx->options->pathname);
  if (ctx->options->body != NULL) {
    free(ctx->options->body);
  }
  free(ctx->options);
  free(ctx);

  // Exit thread
  pthread_exit(NULL);
}

int RequestAsync(HTTPRequest* req, HTTPRequestCallback cb) {
  // NOTE:
  // The AsyncRequestContext contains a full HTTPRequest copy
  // Both are heap allocated and freed by requestAsync();

  int size = 0;

  // Copy HTTPRequest string fields
  char* headers  = copyString(req->headers);
  char* hostname = copyString(req->hostname);
  char* pathname = copyString(req->pathname);
  char* body     = copyString(req->body);

  // Allocate & assign HTTPRequest
  HTTPRequest* copy = calloc(1, sizeof(*copy));
  copy->method      = req->method;
  copy->headers     = headers;
  copy->hostname    = hostname;
  copy->port        = req->port;
  copy->pathname    = pathname;
  copy->body        = body;
  copy->debug       = req->debug;

  // Allocate & assign AsyncRequestContext
  AsyncRequestContext* ctx = calloc(1, sizeof(*ctx));
  ctx->options             = copy;
  ctx->onComplete          = cb;

  // Create request thread
  pthread_t thread;
  if (pthread_create(&thread, NULL, requestAsync, (void*)ctx)) {
    perror("Failed to make thread");
    return -1;
  }
  return 0;
}
