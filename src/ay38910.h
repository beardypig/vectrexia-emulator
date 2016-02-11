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
#ifndef VECTREXIA_AY38910_H
#define VECTREXIA_AY38910_H

#include <stdint.h>

enum {
    PSG_NACT,
    PSG_LATCH_ADDR,
    PSG_IAB,
    PSG_DTS, // read
    PSG_LATCH_BAR,
    PSG_DW,
    PSG_DWS, // write
    PSG_LATCH_INTAK
};

class AY38910
{
    using read_io_callback = uint8_t (*)(intptr_t);
    using store_reg_callback = void (*)(intptr_t, uint8_t);

    uint8_t regs[0xf];
    uint8_t addr;

    // port a/b read callbacks
    store_reg_callback store_reg_func = nullptr;
    intptr_t           store_reg_ref = 0;
    read_io_callback   read_io_func = nullptr;
    intptr_t           read_io_ref = 0;

public:
    void Step(uint8_t bus, uint8_t bc1, uint8_t bc2, uint8_t bdir);

    void SetIOReadCallback(read_io_callback func, intptr_t ref);
    void SetRegStoreCallback(store_reg_callback func, intptr_t ref);

};


#endif //VECTREXIA_AY38910_H
