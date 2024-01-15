all: 
	gcc -std=c99 -o sender.o sender.c
	arm-linux-gnueabihf-gcc receiver.c -o receiver.o

clean:
	rm sender.o
	rm receiver.o
