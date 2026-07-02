CFLAGS += -ansi -Wall -Wextra -pedantic -Wno-unused

all: cssg
clean:
	rm -f cssg *.o
fmt:
	clang-format -i *.c *.h

cssg: expr.o fs.o md.o parser.o tmpl.o types.o
