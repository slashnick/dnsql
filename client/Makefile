CC = clang
WIGNORE = -Wno-missing-variable-declarations -Wno-unused-parameter \
		  -Wno-unused-function
CFLAGS = -Wextra -Werror $(WIGNORE) $(shell python3-config --cflags)
LDFLAGS = $(shell python3-config --ldflags)

SRC = dnsql.c

.PHONY: clean format

dnsql.so: $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -fPIC -shared $^ -o $@

dnsql.dylib: $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -fPIC -dynamiclib $^ -o $@

clean:
	rm -rf *.dylib *.so *.dSYM

format:
	clang-format -i $(SRC)
