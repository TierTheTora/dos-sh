AS=nasm

all: build

build_prog: read_input.asm hello.asm wait.asm get.asm
	$(AS) read_input.asm -o build/read_input.com
	$(AS) hello.asm -o build/hello.com
	$(AS) wait.asm -o build/wait.com
	$(AS) get.asm -o build/get.com


build_dir:
	mkdir -p build

build: build_dir build_prog

clean:
	rm -rf build
