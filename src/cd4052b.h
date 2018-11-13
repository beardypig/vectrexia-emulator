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

#ifndef VECTREXIA_CD4052B_H
#define VECTREXIA_CD4052B_H
#include "via6522.h"

using VIAPorts = VIA6522::PORTS;

struct DACPorts
{
    float v = 0.0f;        // DAC voltage
};

/*
 * CD4052B, Dual 4-Channel Analog Multiplexer/Demultiplexer
 */
class CD4052B {
public:

    // Multiplexer ouputs
    struct PORTS {
        float out1A = 0.0f;    // Y-axis voltage
        float out1B = 0.0f;    // Integrator offset
        float out1C = 0.0f;    // Z-axis voltage
        float out1D = 0.0f;    // Sound
        float out2 = 0.0f;     // Compare
    } ports;

    // Constructor
    CD4052B(VIAPorts * const via_, DACPorts * const dac_);
    
    // Stepping function
    void step();

private:
    VIAPorts *via = nullptr;
    DACPorts *dac = nullptr;
};

#endif // VECTREXIA_CD4052B_H
