COMPILER = clang
COMPILER_FLAGS = -Wpedantic -Wextra -fsanitize=undefined -fsanitize=address -lpthread
INCLUDE = include
SOURCES = src/cli-example.c\
src/hchess.c

cli-example : ${SOURCES}
	${COMPILER} -I${INCLUDE} -o cli ${SOURCES} ${COMPILER_FLAGS}