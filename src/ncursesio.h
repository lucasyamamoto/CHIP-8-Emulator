#ifndef _NCURSESIO_H
#define _NCURSESIO_H

#include "io.h"
#include <ncurses.h>

#define NUM_KEYS 16

class NCursesIO : public IO
{
public:
    NCursesIO();
    ~NCursesIO();

    virtual void draw(const unsigned char* gfx) override;
    virtual void updateKeys() override;
    virtual bool isKeyPressed(unsigned char keyValue) override;
    virtual bool anyKeyPressed();
private:
    bool keyList[NUM_KEYS];
};

#endif  // _NCURSESIO_H