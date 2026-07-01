CFLAGS += -ansi -Wall -Wextra -pedantic -Wno-unused

all: cssg
clean:
	rm -f cssg *.o
fmt:
	clang-format -i *.c *.h

cssg: fs.o parser.o types.o
