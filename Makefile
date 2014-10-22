TARGET = MegaFuse

###############


SRC = src/MegaFuseApp.cpp src/file_cache_row.cpp src/EventsHandler.cpp src/MegaFuse.cpp  src/megafusemodel.cpp src/megaposix.cpp src/Config.cpp src/fuseImpl.cpp src/megacli.cpp src/Logger.cpp

SRC += sdk/megabdb.cpp sdk/megaclient.cpp sdk/megacrypto.cpp 

OUT = $(TARGET)
OBJ = $(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SRC)))

.PHONY:	


# include directories
INCLUDES = -I inc -I /usr/include/cryptopp -I sdk

# C compiler flags (-g -O2 -Wall)
CCFLAGS =   -O0 -g -fstack-protector-all -Wall #-non-call-exceptions
CCFLAGS += $(shell pkg-config --cflags libcurl fuse)
CPPFLAGS =  -std=c++0x $(CCFLAGS) -D_GLIBCXX_DEBUG 

# compiler
CC = gcc
CPP = g++
CXX= g++
# library paths
LIBS = 

# compile flags
LDFLAGS = -lcryptopp -lfreeimage -ldb_cxx
LDFLAGS += $(shell pkg-config --libs libcurl fuse)

megafuse: $(OUT)

all: megafuse

$(OUT): $(OBJ) 
	$(CPP) $(CPPFLAGS) -o $(OUT) $(OBJ) $(LDFLAGS)

.cpp.o:
	$(CPP) $(INCLUDES) $(CPPFLAGS) -c $< -o $@


clean:	
	rm -f $(OBJ) $(OUT)
