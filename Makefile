all: test

test: mjson.h mjson_test.c
	$(CC) mjson_test.c -W -Wall -std=c99 $(CFLAGS) -o /tmp/x && $(DEBUGGER) /tmp/x
	g++ -x c++ mjson_test.c -W -Wall $(CFLAGS) -o /tmp/x && /tmp/x

VC98 = docker run -v $(CURDIR):$(CURDIR) -w $(CURDIR) docker.io/mgos/vc98
VCFLAGS = /nologo /W4 /O1
vc98: mjson_test.c mjson.h
	$(VC98) wine cl mjson_test.c $(TFLAGS) /Fe$@.exe
	$(VC98) wine $@.exe

clean:
	rm -rf test *.exe *.obj *.dSYM
