CC=gcc
SOURCES=src/main.c src/memory.c src/world.c src/gpt.c src/realm.c
CFLAGS=-Iinclude -pthread -lrt
LDFLAGS=-pthread -lrt
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
