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

#include <cstdint>
#include <algorithm>
#include "cd4052b.h"

CD4052B::CD4052B(VIAPorts * const via_, DACPorts * const dac_, JoyPorts * const joy_)
    : via(via_), dac(dac_), joy(joy_) { }

void CD4052B::step()
{
    if (via->pb_sh())
    {
        switch (via->pb_sel0()|(via->pb_sel1() << 1))
        {
        case 0: // Y Axis Sample and Hold between [-5, +5]
            ports_.out1A = dac->Vout * 2;
            ports_.out2 = joy->p1_pot_x;
            break;
        case 1: // Zero reference
            ports_.out1B = dac->Vout * 2;
            ports_.out2 = joy->p1_pot_y;
            break;
        case 2: // Z Axis (brightness) Sample and Hold
            ports_.out1C = std::max(0.0f, -dac->Vout * 2);
            ports_.out2 = joy->p2_pot_x;
            break;
        case 3:
            ports_.out2 = joy->p2_pot_y;
            break;
        default:
            break;
        }

        // TODO: probably buggy
        ports_.out2 > dac->Vout ? via->set_pb_comp() : via->clr_pb_comp();

        // via->PB5 = ports.out2 > dac->Vout; /// ???
        // joystick_compare = (uint8_t)((pot > (porta ^ 0x80)) ? 0x20 : 0);
        // via->PB5 = (ports.out2 > (porta ^ 0x80));
    }
}

CD4052B::ports_t *CD4052B::ports() 
{
    return &ports_;
}