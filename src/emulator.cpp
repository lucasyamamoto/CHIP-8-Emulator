#include "emulator.h"
#include <cstring>
#include <fstream>

#define NUM_GENERAL_REGISTERS 16
#define MEMORY_SIZE 4096
#define NUM_PIXELS (64 * 32)
#define STACK_LEVEL 16
#define NUM_KEYS 16
#define PROGRAM_LOCATION 0x200
#define ADDRESS(instruction) (instruction & 0xfff)
#define REGISTER_X(instruction) ((instruction >> 8) & 0xf)
#define REGISTER_Y(instruction) ((instruction >> 4) & 0xf)
#define SECOND_ARG(instruction) (instruction & 0xff)
#define THIRD_ARG(instruction) (instruction & 0xf)

/////////////////////////////////////////////////////////////////////////

void CHIP8Emulator::run(const std::string& file)
{
    instance().reset();
    instance().load(file);

    while(true)
    {
        instance().runTick();

        instance().updateKeys();
    }
}

CHIP8Emulator& CHIP8Emulator::instance()
{
    static CHIP8Emulator singletonInstance;

    return singletonInstance;
}

/////////////////////////////////////////////////////////////////////////

CHIP8Emulator::CHIP8Emulator()
    : I(0), PC(PROGRAM_LOCATION), delayTimer(0), soundTimer(0), SP(0)
{
    V     = new GeneralRegister[NUM_GENERAL_REGISTERS]{};
    mem   = new unsigned char[MEMORY_SIZE]{};
    gfx   = new unsigned char[NUM_PIXELS]{};
    stack = new unsigned short[STACK_LEVEL]{};
    key   = new unsigned char[NUM_KEYS]{};
}

CHIP8Emulator::CHIP8Emulator(const CHIP8Emulator& other)
    : I(other.I), PC(other.PC), delayTimer(other.delayTimer), 
      soundTimer(other.soundTimer), SP(other.SP)
{
    V     = new GeneralRegister[NUM_GENERAL_REGISTERS];
    mem   = new unsigned char[MEMORY_SIZE];
    gfx   = new unsigned char[NUM_PIXELS];
    stack = new unsigned short[STACK_LEVEL];
    key   = new unsigned char[NUM_KEYS];

    std::memcpy(V    , other.V    , sizeof(GeneralRegister) * NUM_GENERAL_REGISTERS);
    std::memcpy(mem  , other.mem  , sizeof(unsigned char) * MEMORY_SIZE);
    std::memcpy(gfx  , other.gfx  , sizeof(unsigned char) * NUM_PIXELS);
    std::memcpy(stack, other.stack, sizeof(unsigned short) * STACK_LEVEL);
    std::memcpy(key  , other.key  , sizeof(unsigned char) * NUM_KEYS);
}

CHIP8Emulator::CHIP8Emulator(CHIP8Emulator&& other)
    : V(other.V), I(other.I), PC(other.PC), mem(other.mem), 
      gfx(other.gfx), delayTimer(other.delayTimer), 
      soundTimer(other.soundTimer), stack(other.stack), 
      SP(other.SP), key(other.key)
{
    other.V     = nullptr;
    other.mem   = nullptr;
    other.gfx   = nullptr;
    other.stack = nullptr;
    other.key   = nullptr;
}

CHIP8Emulator& CHIP8Emulator::operator=(const CHIP8Emulator& other)
{
    I          = other.I;
    PC         = other.PC;
    delayTimer = other.delayTimer;
    soundTimer = other.soundTimer;
    SP         = other.SP;

    std::memcpy(V    , other.V    , sizeof(GeneralRegister) * NUM_GENERAL_REGISTERS);
    std::memcpy(mem  , other.mem  , sizeof(unsigned char) * MEMORY_SIZE);
    std::memcpy(gfx  , other.gfx  , sizeof(unsigned char) * NUM_PIXELS);
    std::memcpy(stack, other.stack, sizeof(unsigned short) * STACK_LEVEL);
    std::memcpy(key  , other.key  , sizeof(unsigned char) * NUM_KEYS);
}

CHIP8Emulator& CHIP8Emulator::operator=(CHIP8Emulator&& other)
{
    V          = other.V;
    I          = other.I;
    PC         = other.PC;
    mem        = other.mem;
    gfx        = other.gfx;
    delayTimer = other.delayTimer;
    soundTimer = other.soundTimer;
    stack      = other.stack;
    SP         = other.SP;
    key        = other.key;

    other.V     = nullptr;
    other.mem   = nullptr;
    other.gfx   = nullptr;
    other.stack = nullptr;
    other.key   = nullptr;
}

CHIP8Emulator::~CHIP8Emulator()
{
    delete[] V;
    delete[] mem;
    delete[] gfx;
    delete[] stack;
    delete[] key;
}

/////////////////////////////////////////////////////////////////////////

void CHIP8Emulator::load(const std::string& file)
{
    std::ifstream program(file);

    program.read((char *)&mem[PROGRAM_LOCATION], 0xe00);
}

void CHIP8Emulator::runTick()
{
    decodeAndExecute(fetch());
    updateTimers();
}

bool CHIP8Emulator::hasNewFrame() const
{

}

void CHIP8Emulator::updateKeys()
{

}

void CHIP8Emulator::reset()
{
    delete[] V;
    delete[] mem;
    delete[] gfx;
    delete[] stack;
    delete[] key;

    V          = 0;
    I          = 0;
    PC         = PROGRAM_LOCATION;
    delayTimer = 0;
    soundTimer = 0;
    SP         = 0;

    V     = new GeneralRegister[NUM_GENERAL_REGISTERS]{};
    mem   = new unsigned char[MEMORY_SIZE]{};
    gfx   = new unsigned char[NUM_PIXELS]{};
    stack = new unsigned short[STACK_LEVEL]{};
    key   = new unsigned char[NUM_KEYS]{};
}

/////////////////////////////////////////////////////////////////////////

unsigned short CHIP8Emulator::fetch()
{
    unsigned short instruction = ((unsigned short)mem[PC]) << 8 | mem[PC+1];
    PC = (PC + 2) % MEMORY_SIZE;

    return instruction;
}

void CHIP8Emulator::decodeAndExecute(unsigned short instruction)
{
    switch(instruction >> 12)
    {
    case 0x0:
        basicOperations(ADDRESS(instruction));
        break;
    case 0x1:
        jump(ADDRESS(instruction));
        break;
    case 0x2:
        call(ADDRESS(instruction));
        break;
    case 0x3:
        skipEqual(REGISTER_X(instruction), SECOND_ARG(instruction));
        break;
    case 0x4:
        skipNotEqual(REGISTER_X(instruction), SECOND_ARG(instruction));
        break;
    case 0x5:
        skipRegisterEqual(REGISTER_X(instruction), REGISTER_Y(instruction));
        break;
    case 0x6:
        movValue(REGISTER_X(instruction), SECOND_ARG(instruction));
        break;
    case 0x7:
        addValue(REGISTER_X(instruction), SECOND_ARG(instruction));
        break;
    case 0x8:
        registerOperations(REGISTER_X(instruction), REGISTER_Y(instruction), THIRD_ARG(instruction));
        break;
    case 0x9:
        skipRegisterNotEqual(REGISTER_X(instruction), REGISTER_Y(instruction));
        break;
    case 0xa:
        movAddress(ADDRESS(instruction));
        break;
    case 0xb:
        jumpAddress(ADDRESS(instruction));
        break;
    case 0xc:
        rand(REGISTER_X(instruction), SECOND_ARG(instruction));
        break;
    case 0xd:
        draw(REGISTER_X(instruction), REGISTER_Y(instruction), THIRD_ARG(instruction));
        break;
    case 0xe:
        skipByKey(REGISTER_X(instruction), SECOND_ARG(instruction));
        break;
    case 0xf:
        specialOperations(REGISTER_X(instruction), SECOND_ARG(instruction));
        break;
    }
}

void CHIP8Emulator::updateTimers()
{
    updateDelayTimer();
    updateSoundTimer();
}

void CHIP8Emulator::updateDelayTimer()
{
    if (delayTimer > 0)
        delayTimer--;
}

void CHIP8Emulator::updateSoundTimer()
{
    if (soundTimer > 0)
        soundTimer--;
}

/////////////////////////////////////////////////////////////////////////

void CHIP8Emulator::basicOperations(AddressArgument op)
{

}

void CHIP8Emulator::clear()
{

}

void CHIP8Emulator::ret()
{

}

void CHIP8Emulator::jump(AddressArgument address)
{

}

void CHIP8Emulator::call(AddressArgument address)
{

}

void CHIP8Emulator::skipEqual(RegisterIndex x, RegisterArgument n)
{

}

void CHIP8Emulator::skipNotEqual(RegisterIndex x, RegisterArgument n)
{

}

void CHIP8Emulator::skipRegisterEqual(RegisterIndex x, RegisterIndex y)
{

}

void CHIP8Emulator::movValue(RegisterIndex x, RegisterArgument n)
{

}

void CHIP8Emulator::addValue(RegisterIndex x, RegisterArgument n)
{

}

void CHIP8Emulator::registerOperations(RegisterIndex x, RegisterIndex y, RegisterArgument op)
{

}

void CHIP8Emulator::registerMov(RegisterIndex x, RegisterIndex y)
{

}

void CHIP8Emulator::registerOr(RegisterIndex x, RegisterIndex y)
{

}

void CHIP8Emulator::registerAnd(RegisterIndex x, RegisterIndex y)
{

}


void CHIP8Emulator::registerAdd(RegisterIndex x, RegisterIndex y)
{

}

void CHIP8Emulator::registerSub(RegisterIndex x, RegisterIndex y)
{

}

void CHIP8Emulator::registerShiftRight(RegisterIndex x, RegisterIndex y)
{

}

void CHIP8Emulator::registerMinus(RegisterIndex x, RegisterIndex y)
{

}

void CHIP8Emulator::registerShiftLeft(RegisterIndex x, RegisterIndex y)
{

}

void CHIP8Emulator::skipRegisterNotEqual(RegisterIndex x, RegisterIndex y)
{

}

void CHIP8Emulator::movAddress(AddressArgument address)
{

}

void CHIP8Emulator::jumpAddress(AddressArgument address)
{

}

void CHIP8Emulator::rand(RegisterIndex x, RegisterArgument n)
{

}

void CHIP8Emulator::draw(RegisterIndex x, RegisterIndex y, RegisterArgument n)
{

}

void CHIP8Emulator::skipByKey(RegisterIndex x, RegisterArgument op)
{

}

void CHIP8Emulator::skipPressed(RegisterIndex x)
{

}

void CHIP8Emulator::skipNotPressed(RegisterIndex x)
{

}

void CHIP8Emulator::specialOperations(RegisterIndex x, RegisterArgument op)
{

}

void CHIP8Emulator::getDelayTimer(RegisterIndex x)
{

}

void CHIP8Emulator::waitForKey(RegisterIndex x)
{

}

void CHIP8Emulator::setDelayTimer(RegisterIndex x)
{

}

void CHIP8Emulator::setSoundTimer(RegisterIndex x)
{

}

void CHIP8Emulator::addIndex(RegisterIndex x)
{

}

void CHIP8Emulator::getSpriteAddress(RegisterIndex x)
{

}

void CHIP8Emulator::storeDecimal(RegisterIndex x)
{

}

void CHIP8Emulator::storeRegisters(RegisterIndex x)
{

}

void CHIP8Emulator::fillRegisters(RegisterIndex x)
{

}