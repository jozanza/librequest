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
  RequestOptions opts = {
    .method   = RequestMethod_GET,
    .hostname = "postman-echo.com",
    .port     = 443,
    .pathname = "/get?foo1=bar1&foo2=bar2",
  };

  char* res = Request(opts);

  printf("Got response:\n%s\n", res);

  // NOTE:
  // The response is allocated on the heap
  // so be sure to free it once done
  free(res);

  return 0;
}
```

#### Async (Non-blocking) Example

```c
#include <request.h>
#include <stdio.h>

void OnComplete(char* res) {
  printf("Got response:\n%s\n", res);
  // NOTE:
  // The response is automatically cleaned up
  // after the callback returns
}

int main() {
  RequestOptions opts = {
    .method   = RequestMethod_GET,
    .hostname = "postman-echo.com",
    .port     = 443,
    .pathname = "/get?foo1=bar1&foo2=bar2",
  };

  RequestAsync(opts, OnComplete);

  while(1) {
    // This blocks the main thread while the request continues on another thread
    // (use SIGINT (CTRL+C) to kill the program)
  }
}
```
