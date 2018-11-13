#include <cstdint>
#include <algorithm>
#include "cd4052b.h"

CD4052B::CD4052B(VIAPorts * const via_, DACPorts * const dac_)
    : via(via_), dac(dac_) { }

void CD4052B::step()
{
    uint8_t select = (uint8_t)((via->sel1 >> 1) & 0x3);

    if (!(uint8_t)(via->sel1 & 0x1))
    {
        switch (select)
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
