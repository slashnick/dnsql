CC = clang
WIGNORE = -Wno-missing-variable-declarations -Wno-unused-parameter \
		  -Wno-unused-function
CFLAGS = -Weverything -Werror $(WIGNORE)

SRC = dnsql.c

.PHONY: clean format

dnsql.so: dnsql.c
	$(CC) $(CFLAGS) -fPIC -shared $^ -o $@

dnsql.dylib: dnsql.c
	$(CC) $(CFLAGS) -fPIC -dynamiclib $^ -o $@

clean:
	rm -rf *.dylib *.so *.dSYM

format:
	clang-format -i $(SRC)
