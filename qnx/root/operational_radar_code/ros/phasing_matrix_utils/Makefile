OS:=$(shell uname -s)

CC=gcc
CFLAGS=-c
common_libs=-lm
linux_libs=
qnx_libs=-lsocket
INCLUDES=-I"../include/" -I"include/" -I"../tsg/include" -I"../server/include" 
DIR_SOURCES=dio_beam_direction.c _open_PLX9050.c _select_beam.c _select_card.c 
DIR_OBJECTS=$(DIR_SOURCES:.c=.o)
DIR_EXECUTABLE=dio_beam_direction

WRITE_SOURCES=write_card_memory.c _open_PLX9050.c _select_beam.c _select_card.c _write_phase.c 
WRITE_OBJECTS=$(WRITE_SOURCES:.c=.o)
WRITE_EXECUTABLE=write_card_memory

VERIFY_SOURCES=verify_card_memory.c _open_PLX9050.c _select_beam.c _select_card.c _write_phase.c 
VERIFY_OBJECTS=$(VERIFY_SOURCES:.c=.o)
VERIFY_EXECUTABLE=verify_card_memory

all: $(WRITE_EXECUTABLE) $(VERIFY_EXECUTABLE) $(DIR_EXECUTABLE)

$(VERIFY_EXECUTABLE): $(VERIFY_OBJECTS)
ifeq ($(OS),Linux)
	$(CC) -o $@ $(VERIFY_OBJECTS) $(common_libs) $(linux_libs) 
endif
ifeq ($(OS),QNX)
	$(CC) -o $@ $(VERIFY_OBJECTS) $(common_libs) $(qnx_libs) 
endif

$(WRITE_EXECUTABLE): $(WRITE_OBJECTS)
ifeq ($(OS),Linux)
	$(CC) -o $@ $(WRITE_OBJECTS) $(common_libs) $(linux_libs) 
endif
ifeq ($(OS),QNX)
	$(CC) -o $@ $(WRITE_OBJECTS) $(common_libs) $(qnx_libs) 
endif

$(DIR_EXECUTABLE): $(DIR_OBJECTS)
ifeq ($(OS),Linux)
	$(CC) -o $@ $(DIR_OBJECTS) $(common_libs) $(linux_libs) 
endif
ifeq ($(OS),QNX)
	$(CC) -o $@ $(DIR_OBJECTS) $(common_libs) $(qnx_libs) 
endif

.c.o:
	$(CC) $(CFLAGS) $< -o $@ $(INCLUDES)

clean:
	rm -rf $(DIR_OBJECTS) dio_beam_direction
	rm -rf $(WRITE_OBJECTS) write_card_memory 



