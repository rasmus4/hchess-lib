COMPILER = clang
COMPILER_FLAGS = -Wpedantic -Wextra -lpthread
INCLUDE = include
SOURCES = src/main.c\
src/hchess.c

all : ${SOURCES}
	${COMPILER} -I${INCLUDE} -o main ${SOURCES} ${COMPILER_FLAGS}