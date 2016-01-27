/*
Copyright (C) 2016 Beardypig

This file is part of Vectrexia.

Vectrexia is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Vectrexia is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Vectrexia.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef VECTREXIA_WINDOW_H
#define VECTREXIA_WINDOW_H

#include <ncurses.h>
#include <string>

class Window
{
    WINDOW *window_;
    int height, width, y, x;
    std::string title;
    bool border;
public:
    Window(const char *title, int height, int width, int starty, int startx, bool border=true);
    ~Window();
    void Refresh();
    void Clear();
    void mvprint(int y, int x, const char *fmt, ...);
    void vmvprint(int y, int x, const char *fmt, va_list ap);
    void mvprint_attr(int y, int x, int attr, const char *fmt, ...);
    int set_attr_on(int attr);
    int set_attr_off(int attr);
    void move_cursor(int y, int x);
    void hide();

    int getHeight();
};


#endif //VECTREXIA_WINDOW_H
