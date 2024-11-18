

all: src/main.c src/lexer.c src/generator.c
	$(CC) -Wall -Wextra -Werror -Iinclude -o main src/main.c src/lexer.c src/generator.c

clean:
	rm -f main
