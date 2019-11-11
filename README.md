# librequest
A simple async HTTP request library written in pure C

## Installation

```
git clone https://github.com/jozanza/librequest.git
cd librequest
make install
```

## Usage

`librequest` depends on `libopenssl`, so be sure to include the following `LDFLAGS` in your Makefile:

```
LDFLAGS = \
	-I/usr/local/opt/openssl/include \
	-L/usr/local/opt/openssl/lib \
	-lcrypto \
	-lssl
	-lrequest
```