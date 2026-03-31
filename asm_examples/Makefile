AS=nasm

all: build

build_readin: read_input.asm
	$(AS) read_input.asm -o build/read_input.com

build_hello: hello.asm
	$(AS) hello.asm -o build/hello.com

build_dir:
	mkdir -p build

build: build_dir build_readin build_hello

clean:
	rm -rf build
