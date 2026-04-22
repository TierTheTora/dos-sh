CC=gcc
FLAGS=-Wall -Wextra -Wpedantic
LIBS=-lreadline
SRC_DIR=src
BUILD_DIR=build
TARGET_EXE=$(BUILD_DIR)/dos

all: build

build: $(SRC_DIR)/headers/print.h $(SRC_DIR)/print.c \
       $(SRC_DIR)/headers/dos_const.h \
       $(SRC_DIR)/headers/dos_exec.h $(SRC_DIR)/dos_exec.c \
       $(SRC_DIR)/headers/dos_cmds.h $(SRC_DIR)/dos_cmds.c \
       $(SRC_DIR)/headers/parse_opt.h $(SRC_DIR)/parse_opt.c \
       $(SRC_DIR)/headers/conio.h $(SRC_DIR)/conio.c \
       $(SRC_DIR)/headers/dos_lib.h $(SRC_DIR)/dos_lib.c \
       $(SRC_DIR)/headers/trim.h $(SRC_DIR)/trim.c \
       $(SRC_DIR)/main.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(FLAGS) -c $(SRC_DIR)/print.c     -o $(BUILD_DIR)/print.o
	$(CC) $(FLAGS) -c $(SRC_DIR)/dos_cmds.c  -o $(BUILD_DIR)/dos_cmds.o
	$(CC) $(FLAGS) -c $(SRC_DIR)/dos_exec.c  -o $(BUILD_DIR)/dos_exec.o
	$(CC) $(FLAGS) -c $(SRC_DIR)/parse_opt.c -o $(BUILD_DIR)/parse_opt.o
	$(CC) $(FLAGS) -c $(SRC_DIR)/conio.c     -o $(BUILD_DIR)/conio.o
	$(CC) $(FLAGS) -c $(SRC_DIR)/dos_lib.c   -o $(BUILD_DIR)/dos_lib.o
	$(CC) $(FLAGS) -c $(SRC_DIR)/trim.c      -o $(BUILD_DIR)/trim.o
	\
	$(CC) $(FLAGS) $(SRC_DIR)/main.c $(BUILD_DIR)/*.o $(LIBS) -o $(TARGET_EXE)

clean:
	rm -rf $(BUILD_DIR)

run: $(TARGET_EXE)
	./$(TARGET_EXE)
