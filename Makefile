
CC=gcc
CFLAGS= -std=gnu11
LDFLAGS=-lelf -ldl

OBJECTS = $(patsubst src/%.c, build/lib/%.o, $(wildcard src/*.c))
HEADERS = $(wildcard src/*.h)

all: out/libstapsdt.a out/libstapsdt.so

build/lib/libstapsdt-x86_64.o: src/asm/libstapsdt-x86_64.s
	mkdir -p build
	$(CC) $(CFLAGS) -fPIC -c $^ -o $@

build/lib/%.o: src/%.c $(HEADERS)
	mkdir -p build/lib/
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

out/libstapsdt.a: $(OBJECTS) build/lib/libstapsdt-x86_64.o
	mkdir -p out
	ar rcs $@ $^

out/libstapsdt.so: $(OBJECTS) build/lib/libstapsdt-x86_64.o
	mkdir -p out
	$(CC) $(CFLAGS) -shared -o $@ $^

demo: all example/demo.c
	$(CC) $(CFLAGS) example/demo.c out/libstapsdt.a -o demo -Isrc/ $(LDFLAGS)

test: all
	make -C ./tests/

clear:
	rm -rf build/*
	rm -rf out/*

lint:
	clang-tidy src/*.h src/*.c

format:
	clang-tidy src/*.h src/*.c -fix

docs:
	make -C ./docs/ html

docs-server:
	cd docs/_build/html; python3 -m http.server;

.PHONY: all clear lint format build-tests docs
