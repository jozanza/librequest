# librequest

A simple async HTTP request library written in pure C

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

`librequest` supports both blocking (synchronous) and non-blocking (asynchronous) HTTP requests 🎉

#### Blocking Example

To make a synchronous request

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
  char* reponseBody = Request(opts);
  printf("Got response:\n%s\n", responseBody);
  return 0;
}
```

#### Non-blocking Example

To make an asynchronous request

```c
#include <request.h>
#include <stdio.h>

void OnComplete(char* responseBody) {
  printf("Got response:\n%s\n", responseBody);
  exit(0);
}

int main() {
  RequestOptions opts = {
    .method   = RequestMethod_GET,
    .hostname = "postman-echo.com",
    .port     = 443,
    .pathname = "/get?foo1=bar1&foo2=bar2",
  };
  RequestAsync(opts, OnComplete);
  // block the main thread while we wait for the request to complete
  while(1) {}
}
```
