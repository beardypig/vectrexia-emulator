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

#ifndef VECTREXIA_MC1407P8_H
#define VECTREXIA_MC1407P8_H

#include "via6522.h"
#include "veclib.h"

/*
 * CD4052B, Digital to Analog Converter
 */
class MC1408P8
{
public:
    
    // Constructor
    MC1408P8(VIAPorts * via_);

    // Stepping function
    void step();

    // Output voltage
    float Vout = 0.0f;

private:
    vxl::delay<float> delay;
    VIAPorts *via;
};

using DACPorts = MC1408P8;

#endif VECTREXIA_MC1407P8_H
