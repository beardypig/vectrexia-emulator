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

#include "mc1408p8.h"

MC1408P8::MC1408P8(VIAPorts * via_) : via(via_), delay(1, 0.0f) { }

void MC1408P8::step()
{
    delay.step(2.5f - ((via->PA ^ 0x80) * (5.0f / 256.0f)));
    Vout = delay.output;
}
