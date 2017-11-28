
CC=gcc
CFLAGS= -std=gnu11
LDFLAGS=-lelf -ldl -Wl,-z,noexecstack
VERSION=0.1.0

PREFIX=/usr

OBJECTS = $(patsubst src/%.c, build/lib/%.o, $(wildcard src/*.c))
HEADERS = $(wildcard src/*.h)

SOLINK = libstapsdt.so
SONAME = libstapsdt.so.0

all: out/libstapsdt.a out/$(SONAME)

install:
	mkdir -p $(DESTDIR)$(PREFIX)/lib
	mkdir -p $(DESTDIR)$(PREFIX)/include
	cp out/$(SONAME) $(DESTDIR)$(PREFIX)/lib/$(SONAME)
	cp src/libstapsdt.h $(DESTDIR)$(PREFIX)/include/
	ln -s $(DESTDIR)$(PREFIX)/lib/$(SONAME) $(DESTDIR)$(PREFIX)/lib/$(SOLINK)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/lib/$(SONAME)
	rm -f $(DESTDIR)$(PREFIX)/lib/$(SOLINK)
	rm -f $(DESTDIR)$(PREFIX)/include/libstapsdt.h

build/lib/libstapsdt-x86_64.o: src/asm/libstapsdt-x86_64.s
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -c $^ -o $@

build/lib/%.o: src/%.c $(HEADERS)
	mkdir -p build/lib/
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

out/libstapsdt.a: $(OBJECTS) build/lib/libstapsdt-x86_64.o
	mkdir -p out
	ar rcs $@ $^

out/$(SONAME): $(OBJECTS) build/lib/libstapsdt-x86_64.o
	mkdir -p out
	$(CC) $(CFLAGS) -shared -Wl,-soname=$(SONAME) -o $@ $^ $(LDFLAGS)

demo: all example/demo.c
	$(CC) $(CFLAGS) example/demo.c out/libstapsdt.a -o demo -Isrc/ $(LDFLAGS)

test: all
	make -C ./tests/

clean:
	rm -rf build/*
	rm -rf out/*
	make clean -C ./tests/

lint:
	clang-tidy src/*.h src/*.c -- -Isrc/

format:
	clang-tidy src/*.h src/*.c -fix -- -Isrc/

docs:
	make -C ./docs/ html

docs-server:
	cd docs/_build/html; python3 -m http.server;

deb-pkg-setup:
	mkdir -p dist/libstapsdt-$(VERSION)/;
	git archive HEAD | gzip > dist/libstapsdt-$(VERSION).tar.gz;
	tar xvzf dist/libstapsdt-$(VERSION).tar.gz -C dist/libstapsdt-$(VERSION)/;
	cd dist/libstapsdt-$(VERSION); dh_make -l -c mit -y -f ../libstapsdt-$(VERSION).tar.gz;
	rm -rf dist/libstapsdt-$(VERSION)/debian/*.ex  dist/libstapsdt-$(VERSION)/debian/*.EX  dist/libstapsdt-$(VERSION)/debian/README.*


.PHONY: all clear lint format build-tests docs install uninstall deb-pkg-setup
