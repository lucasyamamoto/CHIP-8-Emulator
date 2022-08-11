#ifndef _IO_H
#define _IO_H

class IO
{
public:
    /* Video */
    virtual void draw(const unsigned char *gfx) = 0;

    /* Input */
    virtual void updateKeys() = 0;
    virtual bool isKeyPressed(unsigned char keyValue) = 0;
};

#endif  // _IO_H