all:
	mkdir -p bin
	c++ src/main.cpp src/emulator.cpp src/ncursesio.cpp -o bin/chip8emulator -lncurses