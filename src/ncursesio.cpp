#include "ncursesio.h"
#include <iostream>
#include <cstdlib>

#define NUM_LINES 32
#define NUM_COLUMNS 64
#define NUM_PIXELS (NUM_COLUMNS * NUM_LINES)
#define BLACK_PAIR 1
#define WHITE_PAIR 2

NCursesIO::NCursesIO()
{
    initscr();
    cbreak();
    noecho();

    if(!has_colors())
    {
        endwin();
        std::cerr << "Colors not supported" << std::endl;
        exit(EXIT_FAILURE);
    }

    start_color();
    init_pair(BLACK_PAIR, COLOR_BLACK, COLOR_BLACK);
    init_pair(WHITE_PAIR, COLOR_WHITE, COLOR_WHITE);
}

NCursesIO::~NCursesIO()
{
    endwin();
}

void NCursesIO::draw(const unsigned char* gfx)
{
    int x, y, pair;

    for(int i = 0; i < NUM_PIXELS; i++)
    {
        // Get pixel data
        pair = (gfx[i] ? WHITE_PAIR : BLACK_PAIR);
        x = i % NUM_COLUMNS;
        y = i / NUM_COLUMNS;

        // Draw
        attron(COLOR_PAIR(pair));
        mvprintw(y, x*2, "  ");
        attroff(COLOR_PAIR(pair));
    }

    refresh();
}

void NCursesIO::updateKeys()
{
    
}

bool NCursesIO::isKeyPressed(unsigned char keyValue)
{
    return true;
}

bool NCursesIO::anyKeyPressed()
{
    return true;
}