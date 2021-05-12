all: mousemic

clean:
	rm mousemic

CFLAGS= -Wall -O3 -march=native

mousemic : mousemic.c
	gcc $(CFLAGS) -o mousemic mousemic.c -lSDL -lm -lpthread -lX11
