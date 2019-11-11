#include "request.h"
#include <stdio.h>
#include <stdlib.h>

void OnComplete(HTTPResponse* res) {
  if (res) {
    printf("Async Res Headers:\n%s\n\n", res->headers);
    printf("Async Res Body:\n%s\n\n", res->body);
    FreeResponse(res);
  }
  exit(0);
}

int main() {
  // Synchronous request
  {
    HTTPRequest req = {
        .method   = HTTPRequestMethod_GET,
        .port     = HTTPRequestPort_DEFAULT_HTTPS,
        .headers  = "Accept: */*\r\n",
        .hostname = "postman-echo.com",
        .pathname = "/get?foo1=bar1&foo2=bar2",
    };
    HTTPResponse* res = Request(&req);
    if (res) {
      printf("Sync Res Headers:\n%s\n\n", res->headers);
      printf("Sync Res Body:\n%s\n\n", res->body);
      FreeResponse(res);
    }
  }

  // Asynchronous request
  {
    HTTPRequest req = {
        .method   = HTTPRequestMethod_GET,
        .port     = HTTPRequestPort_DEFAULT_HTTPS,
        .headers  = "Accept: */*\r\n",
        .hostname = "postman-echo.com",
        .pathname = "/get?foo1=bar1&foo2=bar2",
    };
    RequestAsync(&req, OnComplete);
  }

  while (1) {
    // This blocks the main thread while the request continues on another thread
  }
}