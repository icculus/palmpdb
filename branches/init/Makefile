CC = cc
CFLAGS = -g -W -Wall -pedantic
LDFLAGS =

.PHONY: clean all

all: pdbinfo makepdb

pdbinfo: palmpdb.o pdbinfo.o
	$(CC) palmpdb.o pdbinfo.o -o pdbinfo $(LDFLAGS)

makepdb: palmpdb.o makepdb.o
	$(CC) palmpdb.o makepdb.o -o makepdb $(LDFLAGS)

clean:
	rm -f *.o pdbinfo makepdb *~
