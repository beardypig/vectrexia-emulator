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
//#include "debugger.h"
#include <ncurses.h>
#include "debugger.h"

int main(int argc, char *argv[])
{
    int ch;
    Debugger debugger;

    while (!debugger.is_ended())
    {
        debugger.CaptureKey(getch());
        debugger.Run();
    }

    endwin();
    return 0;
}
