LDFLAGS=-lelf -ldl
VALGRINDFLAGS= --tool=memcheck --leak-check=full --error-exitcode=1

all: test-memory-leaks

clean:
	rm -f test-memory-leaks

build-memory-leaks: test-memory-leaks.c
	$(CC) $< ../out/libstapsdt.a -o test-memory-leaks -I../src/ $(LDFLAGS)

test-memory-leaks: build-memory-leaks
	./test-memory-leaks	
	valgrind $(VALGRINDFLAGS) ./test-memory-leaks
