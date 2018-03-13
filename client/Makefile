CC = clang
CFLAGS = $(shell python3-config --cflags)
LDFLAGS = $(shell python3-config --ldflags)

driver: dns_map_c_driver.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	$(RM) driver *.o
