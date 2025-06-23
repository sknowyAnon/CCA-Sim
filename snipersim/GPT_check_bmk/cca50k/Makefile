CC=gcc
SOURCES=src/main.c src/memory.c src/world.c src/gpt.c 
CFLAGS=-Iinclude -pthread
LDFLAGS=-pthread
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=arm_cca_simulation

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

.PHONY: all clean
