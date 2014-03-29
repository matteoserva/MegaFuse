TARGET = MegaFuse

###############


SRC = src/MegaFuse.cpp src/megacli.cpp src/megafusecallbacks.cpp  src/megafusemodel.cpp src/megaposix.cpp src/Config.cpp sdk/megabdb.cpp src/megaclient.cpp sdk/megacrypto.cpp src/fuseImpl.cpp

OUT = $(TARGET)
OBJ = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SRC)))

.PHONY:	


# include directories
INCLUDES = -I inc -I /usr/include/cryptopp

# C compiler flags (-g -O2 -Wall)
CCFLAGS =   -O0 -g -fstack-protector-all -Wall -D_FILE_OFFSET_BITS=64 #-non-call-exceptions
CPPFLAGS =  -std=c++0x $(CCFLAGS) -D_GLIBCXX_DEBUG -D_FILE_OFFSET_BITS=64

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

.c.o:
	$(CC) $(INCLUDES) $(CCFLAGS) -c $< -o $@ 

.cpp.o:
	$(CPP) $(INCLUDES) $(CPPFLAGS) -c $< -o $@


clean:	
	rm -f $(OBJ) $(OUT)
