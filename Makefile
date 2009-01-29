CC = gcc
CFLAGS = -Wall -O2
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

default: cleaner

install: cleaner
	test -d $(BINDIR) || install -d -m 0755 $(BINDIR)
	install -s -m 0755 cleaner $(BINDIR)

clean:
	find . \( -name "*~" -or -name "#*" -or -name ".#*" \) -exec rm {} \;
	rm -f cleaner
