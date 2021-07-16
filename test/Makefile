DEFS = -DMJSON_ENABLE_MERGE=1 -DMJSON_ENABLE_PRETTY=1
WARN ?= -W -Wall -Werror -Wshadow -Wdouble-promotion -fno-common -Wconversion
CFLAGS ?= $(WARN) -g3 -Os -I../src $(DEFS)
GCOVCMD ?= true
PDIR ?= $(realpath $(CURDIR)/..)
VC98 = docker run -v $(PDIR):$(PDIR) -w $(CURDIR) mdashnet/vc98
VCFLAGS = /nologo /W3 /O1 /I../src /MD $(DEFS) $(TFLAGS)
VC2017 = docker run -v $(PDIR):$(PDIR) -w $(CURDIR) mdashnet/vc2017
LIN = docker run -v $(PDIR):$(PDIR) -w $(CURDIR) mdashnet/cc2
ARM = docker run -v $(PDIR):$(PDIR) -w $(CURDIR) mdashnet/armgcc

ifeq ($(shell uname -s),Darwin)
ifeq ($(CC),clang)
	EXTRA = -coverage
	GCOVCMD = gcov unit_test.c ; curl -s https://codecov.io/bash | /bin/bash
endif
endif

all: test linux arm vc98 vc2017

test: ../src/mjson.h ../src/mjson.c unit_test.c
	$(CC) ../src/mjson.c unit_test.c $(CFLAGS) $(EXTRA) -o unit_test && $(DEBUGGER) ./unit_test
	$(CXX) -g -x c++ ../src/mjson.c unit_test.c $(CFLAGS) -o unit_test && $(DEBUGGER) ./unit_test
	@test -z "$(GCOVCMD)" || $(GCOVCMD)

linux: test
linux: CFLAGS += -O2 -fsanitize=address,undefined,shift,null,return,bounds,alignment,object-size,bool,enum -static-libasan
linux: CC = $(LIN) cc
linux: CXX = $(LIN) c++
linux: DEBUGGER = $(LIN)

arm: unit_test.c ../src/mjson.h ../src/mjson.c
	$(ARM) arm-none-eabi-gcc ../src/mjson.c unit_test.c $(CFLAGS) --specs=nosys.specs -o unit_test

vc98: unit_test.c ../src/mjson.h ../src/mjson.c
	$(VC98) wine cl ../src/mjson.c unit_test.c $(VCFLAGS) /Fe$@.exe
	$(VC98) wine $@.exe

vc2017: unit_test.c ../src/mjson.h ../src/mjson.c
	$(VC2017) wine64 cl ../src/mjson.c unit_test.c $(VCFLAGS) /Fe$@.exe
	$(VC2017) wine64 $@.exe

arduino:
	rm -rf /tmp/ArduinoDeviceShadowExample
	cp -r ../examples/ArduinoDeviceShadowExample /tmp/
	cp ../* /tmp/ArduinoDeviceShadowExample
	/Applications/Arduino.app/Contents/MacOS/Arduino --verify /tmp/ArduinoDeviceShadowExample/ArduinoDeviceShadowExample.ino

clean:
	rm -rf unit_test *.exe *.obj *.dSYM
