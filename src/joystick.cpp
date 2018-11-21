/*
Copyright (C) 2016 beardypig, pelorat

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

#include "joystick.h"

void Joystick::step()
{
    // Connects to the PSG
    ports_.io = (p2_joystick.btn_4 << 7) |
               (p2_joystick.btn_3 << 6) |
               (p2_joystick.btn_2 << 5) |
               (p2_joystick.btn_1 << 4) |
               (p1_joystick.btn_4 << 3) |
               (p1_joystick.btn_3 << 2) |
               (p1_joystick.btn_2 << 1) |
               (p1_joystick.btn_1);

    // Connects to the multiplexer
    ports_.p1_pot_x = p1_joystick.pot_x;
    ports_.p1_pot_y = p1_joystick.pot_y;
    ports_.p2_pot_x = p2_joystick.pot_x;
    ports_.p2_pot_y = p2_joystick.pot_y;
}

Joystick::ports_t * Joystick::ports()
{
    return &ports_;
}
