CC     = gcc
CFLAGS = -Wall -Wextra -fsanitize=address -g
NAME   = minimake
SRC    = main.c parse.c exe.c

all:
	${CC} ${CFLAGS} -o ${NAME} ${SRC}

clean:
	rm -f ${NAME}

fclean: clean

re: fclean all

.PHONY: all clean fclean re