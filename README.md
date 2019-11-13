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
  printf("[Status]: %d\n\n", res->status);
  printf("[Headers]:\n%s\n\n", res->headers);
  printf("[Body]:\n%s\n\n", res->body);

  // NOTE: Caller is responsible for freeing the
  // response when making a synchronous request
  FreeResponse(&res);

  return 0;
}
```

#### Async (Non-blocking) Example

```c
#include <request.h>
#include <stdio.h>
#include <unistd.h>

void onComplete(HTTPResponse* res) {
  printf("[Status]: %d\n\n", res->status);
  printf("[Headers]:\n%s\n\n", res->headers);
  printf("[Body]:\n%s\n\n", res->body);

  // NOTE: The response  is automatically freed
  // once the onComplete callback returns
}

int main() {
  HTTPRequest req = {
    .method   = HTTPRequestMethod_GET,
    .port     = HTTPRequestPort_DEFAULT_HTTPS,
    .hostname = "postman-echo.com",
    .pathname = "/get?foo1=bar1&foo2=bar2",
  };

  RequestAsync(req, onComplete);

  sleep(1);
}
```
