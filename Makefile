CC=gcc
FLAGS=-Wall -Wextra

all: build

build: src/print_box.h src/print_box.c \
       src/dos_const.h src/dos_exec.h \
       src/dos_exec.c src/dos_cmds.c \
       src/dos_cmds.h src/dos_lib.h \
       src/dos_lib.c src/parse_opt.h \
       main.c
	mkdir -p build
	$(CC) $(FLAGS) -c src/print_box.c -o build/print_box.o
	$(CC) $(FLAGS) -c src/dos_lib.c -o build/dos_lib.o
	$(CC) $(FLAGS) -c src/dos_cmds.c -o build/dos_cmds.o
	$(CC) $(FLAGS) -c src/dos_exec.c -o build/dos_exec.o
	$(CC) $(FLAGS) -c src/parse_opt.c -o build/parse_opt.o
	$(CC) $(FLAGS) main.c build/*.o -o build/dos

clean:
	rm -rf build
