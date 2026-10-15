CC = gcc
CPPFLAGS = -D_POSIX_C_SOURCE=200809L
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -g -O0

.PHONY: all run clean
all: parent child

parent: parent.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

child: child.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

run: all
	./parent

clean:
	rm -f parent child
