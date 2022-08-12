#include "emulator.h"
#include "ncursesio.h"
#include <cstring>
#include <fstream>

#define NUM_GENERAL_REGISTERS 16
#define MEMORY_SIZE 4096
#define NUM_LINES 32
#define NUM_COLUMNS 64
#define NUM_PIXELS (NUM_COLUMNS * NUM_LINES)
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

        if(instance().hasNewFrame())
            instance().drawFrame();

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
    : I(0), PC(PROGRAM_LOCATION), delayTimer(0), soundTimer(0), SP(0), frameReady(false)
{
    V     = new GeneralRegister[NUM_GENERAL_REGISTERS]{};
    mem   = new unsigned char[MEMORY_SIZE]{};
    gfx   = new unsigned char[NUM_PIXELS]{};
    stack = new unsigned short[STACK_LEVEL]{};
    key   = new unsigned char[NUM_KEYS]{};
    io    = new NCursesIO();

    std::random_device seed;
    randomGenerator = std::mt19937(seed());
    dist = std::uniform_int_distribution<RegisterArgument>();
}

CHIP8Emulator::CHIP8Emulator(const CHIP8Emulator& other)
    : I(other.I), PC(other.PC), delayTimer(other.delayTimer), soundTimer(other.soundTimer), 
      SP(other.SP), randomGenerator(other.randomGenerator), dist(other.dist), frameReady(other.frameReady)
{
    V     = new GeneralRegister[NUM_GENERAL_REGISTERS];
    mem   = new unsigned char[MEMORY_SIZE];
    gfx   = new unsigned char[NUM_PIXELS];
    stack = new unsigned short[STACK_LEVEL];
    key   = new unsigned char[NUM_KEYS];
    io    = other.io;

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
      SP(other.SP), key(other.key), randomGenerator(other.randomGenerator),
      dist(other.dist), frameReady(other.frameReady), io(other.io)
{
    other.V     = nullptr;
    other.mem   = nullptr;
    other.gfx   = nullptr;
    other.stack = nullptr;
    other.key   = nullptr;
    other.io    = nullptr;
}

CHIP8Emulator& CHIP8Emulator::operator=(const CHIP8Emulator& other)
{
    I               = other.I;
    PC              = other.PC;
    delayTimer      = other.delayTimer;
    soundTimer      = other.soundTimer;
    SP              = other.SP;
    randomGenerator = other.randomGenerator;
    dist            = other.dist;
    frameReady      = other.frameReady;
    io              = other.io;

    std::memcpy(V    , other.V    , sizeof(GeneralRegister) * NUM_GENERAL_REGISTERS);
    std::memcpy(mem  , other.mem  , sizeof(unsigned char) * MEMORY_SIZE);
    std::memcpy(gfx  , other.gfx  , sizeof(unsigned char) * NUM_PIXELS);
    std::memcpy(stack, other.stack, sizeof(unsigned short) * STACK_LEVEL);
    std::memcpy(key  , other.key  , sizeof(unsigned char) * NUM_KEYS);

    return *this;
}

CHIP8Emulator& CHIP8Emulator::operator=(CHIP8Emulator&& other)
{
    V               = other.V;
    I               = other.I;
    PC              = other.PC;
    mem             = other.mem;
    gfx             = other.gfx;
    delayTimer      = other.delayTimer;
    soundTimer      = other.soundTimer;
    stack           = other.stack;
    SP              = other.SP;
    key             = other.key;
    randomGenerator = other.randomGenerator;
    dist            = other.dist;
    frameReady      = other.frameReady;
    io              = other.io;

    other.V     = nullptr;
    other.mem   = nullptr;
    other.gfx   = nullptr;
    other.stack = nullptr;
    other.key   = nullptr;
    other.io    = nullptr;

    return *this;
}

CHIP8Emulator::~CHIP8Emulator()
{
    delete[] V;
    delete[] mem;
    delete[] gfx;
    delete[] stack;
    delete[] key;
    delete io;
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
    return frameReady;
}

void CHIP8Emulator::drawFrame()
{
    frameReady = false;
    io->draw(gfx);
}

void CHIP8Emulator::updateKeys()
{

}

void CHIP8Emulator::reset()
{
    I          = 0;
    PC         = PROGRAM_LOCATION;
    delayTimer = 0;
    soundTimer = 0;
    SP         = 0;

    std::memset(V, 0, NUM_GENERAL_REGISTERS);
    std::memset(mem, 0, MEMORY_SIZE);
    std::memset(gfx, 0, NUM_PIXELS);
    std::memset(stack, 0, STACK_LEVEL);
    std::memset(key, 0, NUM_KEYS);
}

/////////////////////////////////////////////////////////////////////////

unsigned short CHIP8Emulator::fetch()
{
    unsigned short instruction = ((unsigned short)mem[PC]) << 8 | mem[PC+1];
    advancePC();

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

void CHIP8Emulator::advancePC()
{
    setPC(PC + 2);
}

void CHIP8Emulator::setPC(SpecialRegister newPC)
{
    PC = newPC % MEMORY_SIZE;
}

/////////////////////////////////////////////////////////////////////////

void CHIP8Emulator::stackPush(unsigned short value)
{
    stack[SP++] = value;
}

unsigned short CHIP8Emulator::stackPop()
{
    return stack[SP--];
}

bool CHIP8Emulator::stackIsFull()
{
    return (SP >= STACK_LEVEL);
}

bool CHIP8Emulator::stackIsEmpty()
{
    return (SP == 0);
}

/////////////////////////////////////////////////////////////////////////

void CHIP8Emulator::basicOperations(AddressArgument op)
{
    switch(op)
    {
    case 0xe0:
        clear();
        break;
    case 0xee:
        ret();
        break;
    }
}

void CHIP8Emulator::clear()
{
    frameReady = true;
    std::memset(gfx, 0, NUM_PIXELS);
}

void CHIP8Emulator::ret()
{
    if(!stackIsEmpty())
        setPC(stackPop());
}

void CHIP8Emulator::jump(AddressArgument address)
{
    setPC(address);
}

void CHIP8Emulator::call(AddressArgument address)
{
    if(!stackIsFull())
    {
        stackPush(PC);
        setPC(address);
    }
}

void CHIP8Emulator::skipEqual(RegisterIndex x, RegisterArgument n)
{
    if(V[x] == n)
        advancePC();
}

void CHIP8Emulator::skipNotEqual(RegisterIndex x, RegisterArgument n)
{
    if(V[x] != n)
        advancePC();
}

void CHIP8Emulator::skipRegisterEqual(RegisterIndex x, RegisterIndex y)
{
    if(V[x] == V[y])
        advancePC();
}

void CHIP8Emulator::movValue(RegisterIndex x, RegisterArgument n)
{
    V[x] = n;
}

void CHIP8Emulator::addValue(RegisterIndex x, RegisterArgument n)
{
    V[x] += n;
}

void CHIP8Emulator::registerOperations(RegisterIndex x, RegisterIndex y, RegisterArgument op)
{
    switch(op)
    {
    case 0x0:
        registerMov(x, y);
        break;
    case 0x1:
        registerOr(x, y);
        break;
    case 0x2:
        registerAnd(x, y);
        break;
    case 0x3:
        registerXor(x, y);
        break;
    case 0x4:
        registerAdd(x, y);
        break;
    case 0x5:
        registerSub(x, y);
        break;
    case 0x6:
        registerShiftRight(x, y);
        break;
    case 0x7:
        registerMinus(x, y);
        break;
    case 0xe:
        registerShiftLeft(x, y);
        break;
    }
}

void CHIP8Emulator::registerMov(RegisterIndex x, RegisterIndex y)
{
    V[x] = V[y];
}

void CHIP8Emulator::registerOr(RegisterIndex x, RegisterIndex y)
{
    V[x] |= V[y];
}

void CHIP8Emulator::registerAnd(RegisterIndex x, RegisterIndex y)
{
    V[x] &= V[y];
}

void CHIP8Emulator::registerXor(RegisterIndex x, RegisterIndex y)
{
    V[x] ^= V[y];
}

void CHIP8Emulator::registerAdd(RegisterIndex x, RegisterIndex y)
{
    unsigned short value = ((unsigned short)V[x]) + ((unsigned short)V[y]);
    
    V[0xf] = (value > 0xffff);
    value &= 0xffff;
    V[x] = value;
}

void CHIP8Emulator::registerSub(RegisterIndex x, RegisterIndex y)
{
    unsigned short value = ((unsigned short)V[x]) - ((unsigned short)V[y]);
    
    V[0xf] = (value <= 0xffff);
    value &= 0xffff;
    V[x] = value;
}

void CHIP8Emulator::registerShiftRight(RegisterIndex x, RegisterIndex y)
{
    V[0xf] = V[x] & 0x01;
    V[x] >>= 1;
}

void CHIP8Emulator::registerMinus(RegisterIndex x, RegisterIndex y)
{
    unsigned short value = ((unsigned short)V[y]) - ((unsigned short)V[x]);
    
    V[0xf] = (value <= 0xffff);
    value &= 0xffff;
    V[x] = value;
}

void CHIP8Emulator::registerShiftLeft(RegisterIndex x, RegisterIndex y)
{
    V[0xf] = V[x] & 0x80;
    V[x] <<= 1;
}

void CHIP8Emulator::skipRegisterNotEqual(RegisterIndex x, RegisterIndex y)
{
    if(V[x] != V[y])
        advancePC();
}

void CHIP8Emulator::movAddress(AddressArgument address)
{
    I = address;
}

void CHIP8Emulator::jumpAddress(AddressArgument address)
{
    setPC(address + V[0]);
}

void CHIP8Emulator::rand(RegisterIndex x, RegisterArgument n)
{
    V[x] = dist(randomGenerator) & n;
}

void CHIP8Emulator::draw(RegisterIndex x, RegisterIndex y, RegisterArgument n)
{
    frameReady = true;

    GeneralRegister xPos = V[x] % NUM_COLUMNS;
    GeneralRegister yPos = V[y] % NUM_LINES;
    V[0xf] = 0;

    // For each row
    for(int i = 0; (i < n) && (yPos < NUM_LINES); i++)
    {
        // Get sprite row
        unsigned char spriteRow = mem[I+i];
        // For each column
        for(int j = 0; (j < 8) && (xPos < NUM_COLUMNS); j++)
        {
            // If bit is on
            if(spriteRow & 1)
            {
                // Detect collision
                if(gfx[xPos + (yPos * NUM_COLUMNS)])
                    V[0xf] = 1;
                // Flip pixel
                gfx[xPos + (yPos * NUM_COLUMNS)] = !gfx[xPos + (yPos * NUM_COLUMNS)];
            }
            // Advance to next bit and next position
            spriteRow >>= 1;
            xPos++;
        }
        // Advance to next position
        xPos = V[x] % NUM_COLUMNS;
        yPos++;
    }
}

void CHIP8Emulator::skipByKey(RegisterIndex x, RegisterArgument op)
{
    switch(op)
    {
    case 0x9e:
        skipPressed(x);
        break;
    case 0xa1:
        skipNotPressed(x);
        break;
    }
}

void CHIP8Emulator::skipPressed(RegisterIndex x)
{

}

void CHIP8Emulator::skipNotPressed(RegisterIndex x)
{

}

void CHIP8Emulator::specialOperations(RegisterIndex x, RegisterArgument op)
{
    switch(op)
    {
    case 0x07:
        getDelayTimer(x);
        break;
    case 0x0a:
        waitForKey(x);
        break;
    case 0x15:
        setDelayTimer(x);
        break;
    case 0x18:
        setSoundTimer(x);
        break;
    case 0x1e:
        addIndex(x);
        break;
    case 0x29:
        getSpriteAddress(x);
        break;
    case 0x33:
        storeDecimal(x);
        break;
    case 0x55:
        storeRegisters(x);
        break;
    case 0x65:
        fillRegisters(x);
        break;
    }
}

void CHIP8Emulator::getDelayTimer(RegisterIndex x)
{
    V[x] = delayTimer;
}

void CHIP8Emulator::waitForKey(RegisterIndex x)
{

}

void CHIP8Emulator::setDelayTimer(RegisterIndex x)
{
    delayTimer = V[x];
}

void CHIP8Emulator::setSoundTimer(RegisterIndex x)
{
    soundTimer = V[x];
}

void CHIP8Emulator::addIndex(RegisterIndex x)
{
    I += V[x];
}

void CHIP8Emulator::getSpriteAddress(RegisterIndex x)
{

}

void CHIP8Emulator::storeDecimal(RegisterIndex x)
{
    GeneralRegister value = V[x];

    for(int i = 0; i < 3; i++)
    {
        mem[I+i] = value % 10;
        value /= 10;
    }
}

void CHIP8Emulator::storeRegisters(RegisterIndex x)
{
    std::memcpy(&mem[I], V, x);
}

void CHIP8Emulator::fillRegisters(RegisterIndex x)
{
    std::memcpy(V, &mem[I], x);
}