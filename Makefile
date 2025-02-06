CC=gcc
CFLAGS=-I. -Wall
DEPS = application_layer.h link_layer.h statemachine.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

main: application_layer.o link_layer.o statemachine.o main.o
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o main
