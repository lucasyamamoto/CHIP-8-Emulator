#include "emulator.h"
#include <iostream>

int main(int argc, char **argv)
{
    if(argc > 1)
        CHIP8Emulator::run(argv[1]);
    else
        std::cerr << "Pass the name of the program file as argument" << std::endl;

    return 0;
}