#include "request.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void OnComplete(HTTPResponse* res) {
  printf("[Status]: %d\n\n", res->status);
  printf("[Headers]:\n%s\n\n", res->headers);
  printf("[Body]:\n%s\n\n", res->body);
}

int main() {
  // Synchronous request
  printf("Synchronous Request\n");
  printf("-------------------\n");
  {
    HTTPRequest req = {
        .method   = HTTPRequestMethod_GET,
        .port     = HTTPRequestPort_DEFAULT_HTTPS,
        .headers  = "Accept: */*\r\n",
        .hostname = "postman-echo.com",
        .pathname = "/get?foo1=bar1&foo2=bar2",
    };
    HTTPResponse* res = Request(&req);
    printf("[Status]: %d\n\n", res->status);
    printf("[Headers]:\n%s\n\n", res->headers);
    printf("[Body]:\n%s\n\n", res->body);
    FreeResponse(&res);
  }

  // Asynchronous request
  printf("Async Request\n");
  printf("-------------\n");
  {
    HTTPRequest req = {
        .method   = HTTPRequestMethod_GET,
        .port     = HTTPRequestPort_DEFAULT_HTTPS,
        .headers  = "Accept: */*\r\n",
        .hostname = "postman-echo.com",
        .pathname = "/get?foo1=bar1&foo2=bar2",
    };
    RequestAsync(&req, OnComplete);
    sleep(1);
  }
}
