# Silence command output by default
.SILENT:

# We're just using make as a task runner
.PHONY: all build run test

# Library name
BIN = request

# C compiler
CC := gcc

# Debugger
DB := lldb

# Source files
SRC := request.c

# Source file & test files
TST := $(SRC) request_test.c

# System libraries (be sure to have these installed)
CFLAGS := \
	-I/usr/local/opt/openssl/include

LDFLAGS := \
	-Wno-unused-command-line-argument \
	-L/usr/local/opt/openssl/lib

# Builds the archive
build:
	clear
	rm -rf $(BIN).o $(BIN).a
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC) -c -o $(BIN).o
	ar rcs lib$(BIN).a $(BIN).o
	echo "Built lib$(BIN).a"

# Copies the library and header to the standard locations
install: build
	cp $(BIN).h /usr/local/include
	cp lib$(BIN).a /usr/local/lib
	echo "Installed lib$(BIN).a"

# Runs test suite
test:
	clear
	$(CC) $(CFLAGS) $(LDFLAGS) -lcrypto -lssl $(TST) -o $(BIN)_test
	./$(BIN)_test
