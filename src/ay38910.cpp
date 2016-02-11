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
#include <stdio.h>
#include "ay38910.h"

void AY38910::Step(uint8_t bus, uint8_t bc1, uint8_t bc2, uint8_t bdir)
{
    switch(bdir << 2 | bc2 << 1 | bc1)
    {
        case PSG_NACT: // disabled
        case PSG_IAB:
        case PSG_DW:
            break;
        case PSG_LATCH_ADDR: // latch address
        case PSG_LATCH_BAR:
        case PSG_LATCH_INTAK:
            // NOTE: address are octal in the documentation
            addr = (uint8_t)(bus & 0xf);
            break;
        case PSG_DWS:
            regs[addr] = bus;
            break;
        case PSG_DTS:
            // read callback
            if (store_reg_func) {
                if (addr == 016) { // read from IO
                    if (read_io_func)
                        store_reg_func(store_reg_ref, read_io_func(read_io_ref));
                }
                else { // read from regular register
                    store_reg_func(store_reg_ref, regs[addr]);
                }
            }
            break;
        default:
            break;
    }
}

void AY38910::SetIOReadCallback(AY38910::read_io_callback func, intptr_t ref)
{
    read_io_func = func;
    read_io_ref = ref;
}

void AY38910::SetRegStoreCallback(AY38910::store_reg_callback func, intptr_t ref)
{
    store_reg_func = func;
    store_reg_ref = ref;
}
