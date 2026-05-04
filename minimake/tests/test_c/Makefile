all: program
program: main.o greet.o
	gcc -o program main.o greet.o
main.o: main.c
	gcc -c main.c
greet.o: greet.c
	gcc -c greet.c
clean:
	rm -f *.o program
