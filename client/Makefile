CC = gcc
WIGNORE = -Wno-missing-variable-declarations -Wno-unused-parameter \
		  -Wno-unused-function
CFLAGS = -fPIC -g -Wall -Wextra -Werror $(WIGNORE) $(shell python3-config --cflags)
LDFLAGS = $(shell python3-config --ldflags) -lsqlite3

SRC = dnsql.c

.PHONY: all clean format

all: dnsql.so demo

dnsql.so: dnsql.o
	$(CC) $(CFLAGS) $(LDFLAGS) -shared $^ -o $@

dnsql.dylib: dnsql.o
	$(CC) $(CFLAGS) $(LDFLAGS) -dynamiclib $^ -o $@

demo: demo.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

clean:
	rm -rf *.dylib *.so *.dSYM *.o demo

format:
	clang-format -i $(SRC)
