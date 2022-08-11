#ifndef _EMULATOR_H

#include "io.h"
#include <string>
#include <random>
#include <functional>

typedef unsigned char GeneralRegister;
typedef unsigned short SpecialRegister;
typedef unsigned char Timer;
typedef unsigned char RegisterIndex;
typedef unsigned char RegisterArgument;
typedef unsigned short AddressArgument;

class CHIP8Emulator
{
public:
    /* Static methods */
    static void run(const std::string& file);
    static CHIP8Emulator& instance();

    /* Constructors, operators, and destructor */
    CHIP8Emulator();
    CHIP8Emulator(const CHIP8Emulator& other);
    CHIP8Emulator(CHIP8Emulator&& other);
    CHIP8Emulator& operator=(const CHIP8Emulator& other);
    CHIP8Emulator& operator=(CHIP8Emulator&& other);
    ~CHIP8Emulator();

    /* Instance methods */
    void load(const std::string& file);
    void runTick();
    bool hasNewFrame() const;
    void drawFrame();
    void updateKeys();
    void reset();
private:
    /* Auxiliary methods */
    unsigned short fetch();
    void decodeAndExecute(unsigned short instruction);
    void updateTimers();
    void updateDelayTimer();
    void updateSoundTimer();
    void advancePC();
    void setPC(SpecialRegister newPC);

    /* Stack operations */
    void stackPush(unsigned short value);
    unsigned short stackPop();
    bool stackIsFull();
    bool stackIsEmpty();

    /* Instruction methods */
    void basicOperations(AddressArgument op);                                       // 0***
    void clear();                                                                   // 00E0
    void ret();                                                                     // 00EE
    void jump(AddressArgument address);                                             // 1NNN
    void call(AddressArgument address);                                             // 2NNN
    void skipEqual(RegisterIndex x, RegisterArgument n);                            // 3XNN
    void skipNotEqual(RegisterIndex x, RegisterArgument n);                         // 4XNN
    void skipRegisterEqual(RegisterIndex x, RegisterIndex y);                       // 5XY0
    void movValue(RegisterIndex x, RegisterArgument n);                             // 6XNN
    void addValue(RegisterIndex x, RegisterArgument n);                             // 7XNN
    void registerOperations(RegisterIndex x, RegisterIndex y, RegisterArgument op); // 8XY*
    void registerMov(RegisterIndex x, RegisterIndex y);                             // 8XY0
    void registerOr(RegisterIndex x, RegisterIndex y);                              // 8XY1
    void registerAnd(RegisterIndex x, RegisterIndex y);                             // 8XY2
    void registerXor(RegisterIndex x, RegisterIndex y);                             // 8XY3
    void registerAdd(RegisterIndex x, RegisterIndex y);                             // 8XY4
    void registerSub(RegisterIndex x, RegisterIndex y);                             // 8XY5
    void registerShiftRight(RegisterIndex x, RegisterIndex y);                      // 8XY6
    void registerMinus(RegisterIndex x, RegisterIndex y);                           // 8XY7
    void registerShiftLeft(RegisterIndex x, RegisterIndex y);                       // 8XYE
    void skipRegisterNotEqual(RegisterIndex x, RegisterIndex y);                    // 9XY0
    void movAddress(AddressArgument address);                                       // ANNN
    void jumpAddress(AddressArgument address);                                      // BNNN
    void rand(RegisterIndex x, RegisterArgument n);                                 // CXNN
    void draw(RegisterIndex x, RegisterIndex y, RegisterArgument n);                // DXYN
    void skipByKey(RegisterIndex x, RegisterArgument op);                           // EX**
    void skipPressed(RegisterIndex x);                                              // EX9E
    void skipNotPressed(RegisterIndex x);                                           // EXA1
    void specialOperations(RegisterIndex x, RegisterArgument op);                   // FX**
    void getDelayTimer(RegisterIndex x);                                            // FX07
    void waitForKey(RegisterIndex x);                                               // FX0A
    void setDelayTimer(RegisterIndex x);                                            // FX15
    void setSoundTimer(RegisterIndex x);                                            // FX18
    void addIndex(RegisterIndex x);                                                 // FX1E
    void getSpriteAddress(RegisterIndex x);                                         // FX29
    void storeDecimal(RegisterIndex x);                                             // FX33
    void storeRegisters(RegisterIndex x);                                           // FX55
    void fillRegisters(RegisterIndex x);                                            // FX65
private:
    /* Registers */
    GeneralRegister* V;
    SpecialRegister I;
    SpecialRegister PC;

    /* Memory */
    unsigned char* mem;
    unsigned char* gfx;

    /* Timers */
    Timer delayTimer;
    Timer soundTimer;

    /* Stack */
    unsigned short* stack;
    SpecialRegister SP;

    /* Keypad */
    unsigned char* key;

    /* Random number generator */
    std::mt19937 randomGenerator;
    std::uniform_int_distribution<RegisterArgument> dist;

    /* Input and output */
    bool frameReady;
    IO* io;
};

#endif      // _EMULATOR_H