#!/bin/bash
mkdir -p test_c
cd test_c

echo -e "\n=== Creating Project ==="
cat << 'C1' > main.c
#include <stdio.h>
void greet();
int main() { greet(); return 0; }
C1

cat << 'C1' > greet.c
#include <stdio.h>
void greet() { printf("Hello World!\n"); }
C1

cat << 'MK' > Makefile
all: program
program: main.o greet.o
	gcc -o program main.o greet.o
main.o: main.c
	gcc -c main.c
greet.o: greet.c
	gcc -c greet.c
clean:
	rm -f *.o program
MK

echo -e "\n=== Running minimake ==="
../../minimake
./program

echo -e "\n=== Running minimake (Up to date check) ==="
../../minimake

echo -e "\n=== Cleaning with minimake ==="
../../minimake clean

echo -e "\n=== Running real make ==="
make
./program

echo -e "\n=== Running real make (Up to date check) ==="
make
