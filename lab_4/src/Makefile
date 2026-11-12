CC = gcc
CPPFLAGS = -D_POSIX_C_SOURCE=200809L
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror -g -O2

.PHONY: all run clean
all: parent child

parent: parent.c ipc.c ipc.h
	$(CC) $(CPPFLAGS) $(CFLAGS) parent.c ipc.c -o $@

child: child.c ipc.c ipc.h
	$(CC) $(CPPFLAGS) $(CFLAGS) child.c ipc.c -o $@

run: all
	./parent

clean:
	rm -f parent child
