#!/bin/bash

# Configuration
TEST_DIR="tests/manual_test_dir"
MINIMAKE="../../minimake"
MAKE="make"

mkdir -p "$TEST_DIR"
cd "$TEST_DIR" || exit 1

# Couleurs
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

run_test() {
    local test_name="$1"
    shift
    local args=("$@")

    echo -e "\n${YELLOW}=== Test: $test_name ===${NC}"
    
    # Execution GNU make
    $MAKE "${args[@]}" > out_make.txt 2>&1
    local code_make=$?

    # Nettoyage entre les deux pour que minimake reconstruise (SAUF si c'est le test "Up to date")
    if [[ "$test_name" != *"Up to date"* ]]; then
        $MAKE clean > /dev/null 2>&1 || rm -f *.o main hello
    fi

    # Execution minimake
    $MINIMAKE "${args[@]}" > out_minimake.txt 2>&1
    local code_minimake=$?

    # Comparaison
    if [ "$code_make" -ne "$code_minimake" ]; then
        echo -e "${RED}[FAIL] Exit codes differ: make=$code_make, minimake=$code_minimake${NC}"
    else
        echo -e "${GREEN}[PASS] Exit codes match: $code_make${NC}"
    fi

    echo "--- minimake output ---"
    cat out_minimake.txt
    echo "-----------------------"
}

# Nettoyage
rm -f *.c *.o main Makefile

# ==============================================================================
# TEST 1: Basic compilation
# ==============================================================================
cat << 'EOF' > main.c
#include <stdio.h>
void say_hello();
int main() {
    say_hello();
    return 0;
}
EOF

cat << 'EOF' > hello.c
#include <stdio.h>
void say_hello() {
    printf("Hello from minimake test!\n");
}
EOF

cat << 'EOF' > Makefile
all: main

main: main.o hello.o
	gcc -o main main.o hello.o

main.o: main.c
	gcc -c main.c

hello.o: hello.c
	gcc -c hello.c

clean:
	rm -f *.o main
EOF

run_test "Full build"
run_test "Up to date check"
run_test "Clean" clean

# ==============================================================================
# TEST 2: Compilation Error
# ==============================================================================
cat << 'EOF' > main.c
#include <stdio.h>
int main() {
    printf("Missing semicolon")
    return 0;
}
EOF

run_test "Compilation Error" main.o

# ==============================================================================
# TEST 3: Execution Error (recipe fails)
# ==============================================================================
cat << 'EOF' > Makefile
fail_rule:
	false
	echo "This should not be printed"
EOF

run_test "Recipe failure" fail_rule

# ==============================================================================
# TEST 4: Variables & Multiple spaces
# ==============================================================================
cat << 'EOF' > Makefile
CC=gcc
CFLAGS = -Wall -Wextra 
SRC = hello.c

hello:
	${CC} ${CFLAGS} -o hello ${SRC}
EOF

# Correct hello.c for this test
cat << 'EOF' > hello.c
#include <stdio.h>
int main() { printf("Hello!\n"); return 0; }
EOF

run_test "Variables and compilation" hello
./hello
rm -f hello

echo -e "\n${YELLOW}=== Tests finis ===${NC}"
