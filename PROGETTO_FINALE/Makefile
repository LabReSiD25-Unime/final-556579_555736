CC = gcc
CFLAGS = -g -Wall
TARGET = server_http
OBJS = server.o inizializzazione_server.o gestione_client.o http.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean check