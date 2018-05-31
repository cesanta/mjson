all: test

test: mjson.h mjson_test.c
	cc mjson_test.c -W -Wall -std=c99 $(CFLAGS) -o /tmp/x && /tmp/x
	g++ -x c++ mjson_test.c -W -Wall $(CFLAGS) -o /tmp/x && /tmp/x
