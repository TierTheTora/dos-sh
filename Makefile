CC=gcc
FLAGS=-Wall -Wextra -g
LIBS=-lreadline

all: build

build: src/headers/print.h src/print.c \
       src/headers/dos_const.h \
       src/headers/dos_exec.h src/dos_exec.c \
       src/headers/dos_cmds.h src/dos_cmds.c \
       src/headers/parse_opt.h src/parse_opt.c \
       src/headers/conio.h src/conio.c \
       src/headers/dos_lib.h src/dos_lib.c \
       src/main.c
	mkdir -p build
	$(CC) $(FLAGS) -c src/print.c -o build/print.o
	$(CC) $(FLAGS) -c src/dos_cmds.c -o build/dos_cmds.o
	$(CC) $(FLAGS) -c src/dos_exec.c -o build/dos_exec.o
	$(CC) $(FLAGS) -c src/parse_opt.c -o build/parse_opt.o
	$(CC) $(FLAGS) -c src/conio.c -o build/conio.o
	$(CC) $(FLAGS) -c src/dos_lib.c -o build/dos_lib.o
	$(CC) $(FLAGS) src/main.c build/*.o $(LIBS) -o build/dos

clean:
	rm -rf build
