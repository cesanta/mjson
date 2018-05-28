all: mjson.h mjson_test.c
	$(CC) mjson_test.c -W -Wall -o /tmp/x && /tmp/x
