CC = clang
WIGNORE = -Wno-missing-variable-declarations -Wno-unused-parameter \
		  -Wno-unused-function
CFLAGS = -g -Wall -Wextra -Werror $(WIGNORE) $(shell python3-config --cflags)
LDFLAGS = $(shell python3-config --ldflags)

SRC = dnsql.c

.PHONY: clean format

dnsql.so: dnsql.o
	$(CC) $(LDFLAGS) -fPIC -shared $^ -o $@

dnsql.dylib: dnsql.o
	$(CC) $(LDFLAGS) -fPIC -dynamiclib $^ -o $@

stuff: dnsql.o stuff.o
	$(CC) $(LDFLAGS) -lsqlite3 $^ -o $@

clean:
	rm -rf *.dylib *.so *.dSYM *.o

format:
	clang-format -i $(SRC)
