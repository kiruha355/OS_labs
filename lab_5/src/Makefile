CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror -O2

.PHONY: all clean
all: linked dynamic libfirst.so libsecond.so

linked: linked.c commands.c commands.h contracts.h libfirst.so
	$(CC) $(CFLAGS) linked.c commands.c -L. -lfirst -Wl,-rpath,'$$ORIGIN' -o $@

dynamic: dynamic.c commands.c commands.h contracts.h | libfirst.so libsecond.so
	$(CC) $(CFLAGS) dynamic.c commands.c -ldl -o $@

libfirst.so: first.c contracts.h methods.h
	$(CC) $(CFLAGS) -fPIC -shared first.c -lm -o $@

libsecond.so: second.c contracts.h methods.h
	$(CC) $(CFLAGS) -fPIC -shared second.c -lm -o $@

clean:
	rm -f linked dynamic libfirst.so libsecond.so
