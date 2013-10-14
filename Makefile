CXX=g++
CFLAGS=-g -Wall -isystem /usr/include/cryptopp -std=c++0x -D_FILE_OFFSET_BITS=64
LIB=-lcryptopp -lfreeimage -lcurl -ldb_cxx -lfuse
LDFLAGS=
SOURCES=megaposix.cpp megacli.cpp megaclient.cpp megacrypto.cpp megabdb.cpp megafuse.cpp fuseImpl.cpp fuseFileCache.cpp megafusecallbacks.cpp
OBJECTS=$(SOURCES:.cpp=.o) fuse.o
PROGS=megaclient

all: $(PROGS)

megaclient: $(OBJECTS)
	$(CXX) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIB)

.cpp.o:
	$(CXX) -c $(CFLAGS) $< -o $@

.c.o:
	$(CC)  -c $(CFLAGS) $< -o $@
clean:
	rm -f $(OBJECTS) $(PROGS) $(PROGS:=.o)
