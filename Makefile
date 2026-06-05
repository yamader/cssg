CFLAGS += -ansi -Wall -Wextra -pedantic

all: cssg
clean:
	rm -f cssg *.o
fmt:
	clang-format -i *.c *.h

cssg: parser.o template.o types.o
