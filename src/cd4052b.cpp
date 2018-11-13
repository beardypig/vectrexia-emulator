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

CD4052B::CD4052B(VIAPorts * const via_, DACPorts * const dac_)
    : via(via_), dac(dac_) { }

void CD4052B::step()
{
    if (via->PB0)
    {
        switch ((via->PB1 ? 1 : 0) + (via->PB2 ? 2 : 0))
        {
        case 0: // Y Axis Sample and Hold between [-5, +5]
            ports.out1A = dac->v * 2;
            break;
        case 1: // Zero reference
            ports.out1B = dac->v * 2;
            break;
        case 2: // Z Axis (brightness) Sample and Hold
            ports.out1C = std::max(0.0f, -dac->v * 2);
            break;
        default:
            break;
        }
    }
}
