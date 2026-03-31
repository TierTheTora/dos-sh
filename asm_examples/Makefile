AS=nasm

all: build

build_readin: read_input.asm
	$(AS) read_input.asm -o build/read_input.com

build_hello: hello.asm
	$(AS) hello.asm -o build/hello.com

build: build_readin build_hello
	mkdir -p build

clean:
	rm -rf build
