

all: src/main.c
	$(CC) -Wall -Wextra -Werror -Iinclude -o main src/main.c

clean:
	rm -f main
