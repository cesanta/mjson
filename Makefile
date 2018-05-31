all: test

test: mjson.h mjson_test.c
	$(CC) mjson_test.c -W -Wall -std=c99 $(CFLAGS) -o /tmp/x && /tmp/x
