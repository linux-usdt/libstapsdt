CC=gcc
CFLAGS=
LDFLAGS=-lelf -ldl

all: out/libstapsdt.a

build/libstapsdt-x86_64.o: src/asm/libstapsdt-x86_64.s
	mkdir -p build
	$(CC) $(CFLAGS) -c $^ -o $@

build/libstapsdt.o: src/libstapsdt.c
	mkdir -p build
	$(CC) $(CFLAGS) -c $^ -o $@

build/sdtnote.o: src/sdtnote.c
	mkdir -p build
	$(CC) $(CFLAGS) -c $^ -o $@

build/util.o: src/util.c
	mkdir -p build
	$(CC) $(CFLAGS) -c $^ -o $@

build/string-table.o: src/string-table.c
	mkdir -p build
	$(CC) $(CFLAGS) -c $^ -o $@

out/libstapsdt.a: build/libstapsdt-x86_64.o build/libstapsdt.o build/util.o build/sdtnote.o build/string-table.o
	mkdir -p out
	ar rcs $@ $^

demo: all demo/demo.c
	$(CC) -c demo/demo.c -o build/demo.o -Isrc/
	$(CC) build/demo.o out/libstapsdt.a -o out/demo $(LDFLAGS)

clear:
	rm -rf build/*
	rm -rf out/*
