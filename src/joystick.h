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

#ifndef JOYSTICK_H
#define JOYSTICK_H
#include <cstdint>

class Joystick
{
public:
    struct pins_t {
        float pot_x;
        float pot_y;
        uint8_t btn_1;
        uint8_t btn_2;
        uint8_t btn_3;
        uint8_t btn_4;
    };

    struct ports_t {
        float p1_pot_x;
        float p1_pot_y;
        float p2_pot_x;
        float p2_pot_y;
        uint8_t io;
    };
   
    pins_t p1_joystick;
    pins_t p2_joystick;
    ports_t ports;

    void step();
};

using JoyPorts = Joystick::ports_t;

#endif