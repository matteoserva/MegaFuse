TARGET = MegaFuse

###############


SRC = src/file_cache_row.cpp src/EventsHandler.cpp src/MegaFuse.cpp src/megafusecallbacks.cpp  src/megafusemodel.cpp src/megaposix.cpp src/Config.cpp src/fuseImpl.cpp src/megacli.cpp

SRC += sdk/megabdb.cpp sdk/megaclient.cpp sdk/megacrypto.cpp 

OUT = $(TARGET)
OBJ = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SRC)))

.PHONY:	


# include directories
INCLUDES = -I inc -I /usr/include/cryptopp -I sdk

# C compiler flags (-g -O2 -Wall)
CCFLAGS =   -O0 -g -fstack-protector-all -Wall -D_FILE_OFFSET_BITS=64 #-non-call-exceptions
CPPFLAGS =  -std=c++0x $(CCFLAGS) -D_GLIBCXX_DEBUG

# compiler
CC = gcc
CPP = g++
CXX= g++
# library paths
LIBS = 

# compile flags
LDFLAGS = -lcryptopp -lfreeimage -lcurl -ldb_cxx -lfuse

megafuse: $(OUT)

all: megafuse

$(OUT): $(OBJ) 
	$(CPP) $(CPPFLAGS) -o $(OUT) $(OBJ) $(LDFLAGS)

#.c.o:
#	$(CC) $(INCLUDES) $(CCFLAGS) -c $< -o $@ 

.cpp.o:
	$(CPP) $(INCLUDES) $(CPPFLAGS) -c $< -o $@


clean:	
	rm -f $(OBJ) $(OUT)
