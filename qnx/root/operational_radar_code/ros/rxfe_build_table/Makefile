OS:=$(shell uname -s)

CC=gcc
CFLAGS=-c
common_libs=-lm
linux_libs=
qnx_libs=-lsocket
INCLUDES=-I"../include/" -I"include/" -I"../tsg/include" -I"../server/include" 
SOURCES=main.c _open_PLX9050.c   _rxfe_commands.c 
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=rxfe_build_table

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
ifeq ($(OS),Linux)
	$(CC) -o $@ $(OBJECTS) $(common_libs) $(linux_libs) 
endif
ifeq ($(OS),QNX)
	$(CC) -o $@ $(OBJECTS) $(common_libs) $(qnx_libs) 
endif

.c.o:
	$(CC) $(CFLAGS) $< -o $@ $(INCLUDES)

clean:
	rm -rf $(OBJECTS) plx9050_tcp_driver



