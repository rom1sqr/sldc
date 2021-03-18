VERSION:=1.0.0
GLIB2:=$(shell pkg-config --cflags glib-2.0)
PYTH3:=$(shell pkg-config --cflags python3)
CFLAGS:=-Werror -Wall -pedantic -fPIC

.PHONY: clean build test install uninstall

test: build
	[ "$$(echo 'test string' | ./sldc -c | ./sldc -d)" = "$$(echo 'test string')" ]
	python -c 'import sldc ; print(hex(sldc.version))'
	python -c 'import sldc ; print(sldc.compress(b"test string"))'
	python -c 'import sldc ; print(sldc.decompress(b"\xff\xb3\xa3\x2b\x9b\xa1\x03\x9b\xa3\x93\x4b\x73\x3f\xfd\x00\x00"))'
	python -c 'import sldc ; x = b"test string" ; assert(sldc.decompress(sldc.compress(x)) == x)'
build: sldc libsldc.so _sldc.so sldc.1.gz sldc.h.3.gz LICENSE

sldc: sldc.c
	LANG=C gcc $(CFLAGS) $(GLIB2)          -lsldc  -o $@ $^
libsldc.so: libsldc.c
	LANG=C gcc $(CFLAGS) $(GLIB2)          -shared -o $@ $^
_sldc.so: libsldc.c libsldc_wrap.c
	LANG=C gcc $(CFLAGS) $(GLIB2) $(PYTH3) -shared -o $@ $^
libsldc_wrap.c: libsldc.i
	swig -python -globals _cvar $<
%.gz: %
	gzip -k $<
LICENSE:
	wget -q -O $@ https://www.gnu.org/licenses/gpl-3.0.txt
clean:
	rm -rf __pycache__ sldc *.so *.py *.gz *_wrap.c
install: build
	mkdir -p /usr/include/sldc /usr/share/licenses/sldc
	cp sldc        /usr/bin
	cp sldc.h      /usr/include/sldc
	cp libsldc.so  /usr/lib/libsldc.so.$(VERSION)
	ln -sf         /usr/lib/libsldc.so.$(VERSION) /usr/lib/libsldc.so
	cp libsldc.pc  /usr/lib/pkgconfig
	cp LICENSE     /usr/share/licenses/sldc
	cp sldc.1.gz   /usr/share/man/man1
	cp sldc.h.3.gz /usr/share/man/man3
uninstall:
	rm     /usr/bin/sldc
	rm -rf /usr/include/sldc
	rm -rf /usr/lib/libsldc.so*
	rm     /usr/lib/pkgconfig/libsldc.pc
	rm -rf /usr/share/licenses/sldc
	rm     /usr/share/man/man1/sldc*
	rm     /usr/share/man/man3/sldc*

