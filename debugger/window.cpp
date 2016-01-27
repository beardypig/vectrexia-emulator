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
#include "window.h"

Window::Window(const char *title, int height, int width, int y, int x, bool border)
{
    this->height = height;
    this->width = width;
    this->x = x;
    this->y = y;
    this->title = std::string(title);
    this->border = border;
    window_ = newwin(height, width, y, x);
}

void Window::Refresh()
{
    wrefresh(window_);
}

void Window::Clear()
{
    werase(window_);
    if (border)
    {
        box(window_, 0, 0);
        mvwprintw(window_, 0, 2, title.c_str());
    }
}

Window::~Window()
{
    // clear the border
    wborder(window_, ' ', ' ', ' ',' ',' ',' ',' ',' ');
    wrefresh(window_);
    delwin(window_);
}

void Window::mvprint(int y, int x, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    this->vmvprint(y, x, fmt, ap);
    va_end(ap);
}

int Window::set_attr_on(int attr)
{
    return wattron(window_, attr);
}

int Window::set_attr_off(int attr)
{
    return wattroff(window_, attr);
}

void Window::mvprint_attr(int y, int x, int attr, const char *fmt, ...)
{
    set_attr_on(attr);
    va_list ap;
    va_start(ap, fmt);
    this->vmvprint(y, x, fmt, ap);
    va_end(ap);
    set_attr_off(attr);
}

void Window::vmvprint(int y, int x, const char *fmt, va_list ap)
{

    va_list ap2;
    va_copy(ap2, ap);
    std::string out;

    int size = vsnprintf(NULL, 0, fmt, ap);
    if (size > 0) {
        out = std::string(size+1, 0);
        vsnprintf(&out[0], out.size(), fmt, ap2);
        out.resize(static_cast<size_t>(size));
        mvwprintw(window_, y, x, out.c_str());
    }

    va_end(ap2);
}

int Window::getHeight()
{
    return height;
}

void Window::move_cursor(int y, int x)
{
    wmove(window_, y, x);
}

void Window::hide()
{
    werase(window_);
    wborder(window_, ' ', ' ', ' ',' ',' ',' ',' ',' ');
    wrefresh(window_);
}
