

all: src/main.c src/lexer.c
	$(CC) -Wall -Wextra -Werror -Iinclude -o main src/main.c src/lexer.c -g -ggdb

clean:
	rm -f main
