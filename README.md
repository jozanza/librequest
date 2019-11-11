# librequest

A simple HTTP request library written in pure C

## Installation

```
git clone https://github.com/jozanza/librequest.git
cd librequest
make install
```

## Usage

### Linking

`librequest` depends on `libopenssl`, so you will have to add the following flags to your compiler:

```
-I/usr/local/opt/openssl/include
-L/usr/local/opt/openssl/lib
-lcrypto
-lssl
-lrequest
```

### Making requests

`librequest` supports both synchronous (blocking) and async (non-blocking) HTTP requests 🎉

#### Synchronous (Blocking) Example

```c
#include <request.h>
#include <stdio.h>

int main() {
  HTTPRequest req = {
    .method   = HTTPRequestMethod_GET,
    .port     = HTTPRequestPort_DEFAULT_HTTPS,
    .hostname = "postman-echo.com",
    .pathname = "/get?foo1=bar1&foo2=bar2",
  };

  HTTPResponse* res = Request(req);
  if (res) {
    printf("Got response:\n%s\n", res->body);
    FreeResponse(res);
  }
  return 0;
}
```

#### Async (Non-blocking) Example

```c
#include <request.h>
#include <stdio.h>
#include <stdlib.h>

void OnComplete(HTTPResponse* res) {
  if (res) {
    printf("Got response:\n%s\n", res->body);
    FreeResponse(res);
  }
  exit(0);
}

int main() {
  HTTPRequest req = {
    .method   = HTTPRequestMethod_GET,
    .port     = HTTPRequestPort_DEFAULT_HTTPS,
    .hostname = "postman-echo.com",
    .pathname = "/get?foo1=bar1&foo2=bar2",
  };

  RequestAsync(req, OnComplete);

  while(1) {
    // This blocks the main thread while the request continues on another thread
  }
}
```
