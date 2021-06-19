COMPILER = clang
COMPILER_FLAGS = -Wpedantic -Wextra -fsanitize=undefined -fsanitize=address -lpthread
INCLUDE = include

lib/hchess-lib.o : src/hchess.c include/hchess.h include/protocol.h
	${COMPILER} -I${INCLUDE} -o lib/hchess-lib.o -c src/hchess.c

out/cli-example : lib/hchess-lib.o src/cli-example.c
	${COMPILER} -I${INCLUDE} -o out/cli-example src/cli-example.c lib/hchess-lib.o ${COMPILER_FLAGS}

clean :
	rm -f lib/hchess-lib.o
	rm -f out/cli-example