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
static void serializeRequest(RequestOptions* opts, char req[API_REQUEST_MAX_BUFFER_SIZE + 1]) {
  char* version = "1.0";
  char* methodName;
  switch (opts->method) {
  case RequestMethod_GET:
    methodName = "GET";
    break;
  case RequestMethod_DELETE:
    methodName = "DELETE";
    break;
  case RequestMethod_PATCH:
    methodName = "PATCH";
    break;
  case RequestMethod_POST:
    methodName = "POST";
    break;
  case RequestMethod_PUT:
    methodName = "PUT";
    break;
  }
  if (RequestMethod_GET == opts->method) {
    snprintf(
        req,
        API_REQUEST_MAX_BUFFER_SIZE,
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
        API_REQUEST_MAX_BUFFER_SIZE,
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
        opts->headers,
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

char* Request(RequestOptions* opts) {
  if (!opts) {
    perror("No RequestOptions provided to Request()");
    return NULL;
  }
  if (opts->debug)
    printf("----------------\n");

  // Will be allocated and filled with response body
  char* out = NULL;

  // Create socket and init SSL
  int sock = SocketConnect(opts->hostname, opts->port);
  if (sock == -1) {
    goto done;
  }
  SSL_CTX* sslctx = InitSSLContext();
  SSL* ssl        = SSL_new(sslctx);
  if (!ssl) {
    goto done;
  }
  SSL_set_tlsext_host_name(ssl, opts->hostname);
  SSL_set_fd(ssl, sock);
  if (SSL_connect(ssl) <= 0) {
    // Error occurred, log and close down ssl
    ERR_print_errors_fp(stderr);
    goto done;
  }

  // Send request
  char req[API_REQUEST_MAX_BUFFER_SIZE + 1];
  serializeRequest(opts, req);
  if (opts->debug) {
    printf("Sending request:\n%s\n", req);
    printf("...\n");
  }
  size_t reqLen        = strlen(req);
  ssize_t bytesWritten = SSL_write(ssl, req, reqLen);
  if (bytesWritten < reqLen) {
    perror("Couldn't write to socket\n");
    goto done;
  }
  if (opts->debug) {
    printf("Request sent!\n");
    printf("...\n");
  }

  // Read response
  char res[API_RESPONSE_MAX_BUFFER_SIZE + 1];
  bzero(res, API_REQUEST_MAX_BUFFER_SIZE + 1);
  if (opts->debug) {
    printf("Awaiting response...\n");
  }
  ssize_t bytesRead;
  while ((bytesRead = SSL_read(ssl, res, API_REQUEST_MAX_BUFFER_SIZE)) != 0) {
    if (opts->debug)
      printf("Read %lu bytes...", strlen(res));
  }

  // Split response body from response headers
  char* json = split(res, "\r\n\r\n");
  if (opts->debug) {
    printf("\n...\n");
    printf("[HEADER]\n\n%s\n\n", res);
    printf("[BODY]\n\n%s\n\n", json);
  }

  // Allocate & return response body
  out = malloc(strlen(json) + 1);
  strcpy(out, json);

done:
  // Destroy socket
  shutdown(sock, SHUT_RDWR);
  close(sock);

  // Destroy SSL
  SSL_CTX_free(sslctx);

  // Return response body
  return out;
}

//------------------------------------------------------------------------------
// Async HTTP Requests
//------------------------------------------------------------------------------

// This struct holds HTTP request options and an onComplete function pointer
typedef struct AsyncRequestContext {
  RequestOptions* options;
  RequestCompleteCallback onComplete;
} AsyncRequestContext;

// The pthread that handles the asynchronous request
static void* requestAsync(void* any) {
  // Convert thread arg to AsyncRequestContext
  AsyncRequestContext* ctx = any;

  // Make Request
  char* res = Request(ctx->options);
  ctx->onComplete(res);

  // Free allocated memory
  free(res);
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

int RequestAsync(RequestOptions* opts, RequestCompleteCallback onComplete) {
  // NOTE:
  // The AsyncRequestContext contains a full RequestOptions copy
  // Both are heap allocated and freed by requestAsync();

  int size = 0;

  // Copy RequestOptions string fields
  char* headers = NULL;
  if (opts->headers) {
    size    = strlen(opts->headers) + 1;
    headers = malloc(size);
    strlcpy(headers, opts->headers, size);
  }
  char* hostname = NULL;
  if (opts->hostname) {
    size     = strlen(opts->hostname) + 1;
    hostname = malloc(size);
    strlcpy(hostname, opts->hostname, size);
  }
  char* pathname = NULL;
  if (opts->pathname) {
    size     = strlen(opts->pathname) + 1;
    pathname = malloc(size);
    strlcpy(pathname, opts->pathname, size);
  }
  char* body = NULL;
  if (opts->body) {
    size = strlen(opts->body) + 1;
    body = malloc(size);
    strlcpy(body, opts->body, size);
  }

  // Allocate & assign RequestOptions
  RequestOptions* copy = calloc(1, sizeof(*copy));
  copy->method         = opts->method;
  copy->headers        = headers;
  copy->hostname       = hostname;
  copy->port           = opts->port;
  copy->pathname       = pathname;
  copy->body           = body;
  copy->debug          = opts->debug;

  // Allocate & assign AsyncRequestContext
  AsyncRequestContext* ctx = calloc(1, sizeof(*ctx));
  ctx->options             = copy;
  ctx->onComplete          = onComplete;

  // Create request thread
  pthread_t thread;
  if (pthread_create(&thread, NULL, requestAsync, (void*)ctx)) {
    perror("Failed to make thread");
    return -1;
  }
  return 0;
}
