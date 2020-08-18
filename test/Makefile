all: test
DEFS = -DMJSON_ENABLE_MERGE=1 -DMJSON_ENABLE_PRETTY=1
CFLAGS ?= -g -W -Wall -I../src $(DEFS)
GCOVCMD ?= true

ifeq ($(shell uname -s),Darwin)
ifeq ($(CC),clang)
	EXTRA = -coverage
	GCOVCMD = gcov unit_test.c ; curl -s https://codecov.io/bash | /bin/bash
endif
endif

test: ../src/mjson.h ../src/mjson.c unit_test.c
	$(CC) ../src/mjson.c unit_test.c -std=c99 $(CFLAGS) $(EXTRA) -o unit_test && $(DEBUGGER) ./unit_test
	g++ -g -x c++ ../src/mjson.c unit_test.c $(CFLAGS) -o unit_test && ./unit_test
	@test "$(GCOVCMD)" == true || $(GCOVCMD)

PDIR ?= $(realpath $(CURDIR)/..)
VC98 = docker run -v $(PDIR):$(PDIR) -w $(CURDIR) docker.io/mgos/vc98
VCFLAGS = /nologo /W3 /O1 /I../src $(DEFS) $(TFLAGS)
vc98: unit_test.c ../src/mjson.h ../src/mjson.c
	$(VC98) wine cl ../src/mjson.c unit_test.c $(VCFLAGS) /Fe$@.exe
	$(VC98) wine $@.exe

arduino:
	rm -rf /tmp/ArduinoDeviceShadowExample
	cp -r ../examples/ArduinoDeviceShadowExample /tmp/
	cp ../src/* /tmp/ArduinoDeviceShadowExample
	/Applications/Arduino.app/Contents/MacOS/Arduino --verify /tmp/ArduinoDeviceShadowExample/ArduinoDeviceShadowExample.ino

clean:
	rm -rf unit_test *.exe *.obj *.dSYM
