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
#include <string>
#include <bitset>
#include <memory>
#include <array>
#include "vectrexia.h"
#include "cartridge.h"


const char *Vectrex::GetName()
{
    return kName_;
}

const char *Vectrex::GetVersion()
{
    return kVersion_;
}

void Vectrex::Reset()
{

}

long Vectrex::Run(long cycles)
{
    return cycles;
}

bool Vectrex::LoadCartridge(const uint8_t *data, size_t size)
{
    cartridge_ = std::unique_ptr<Cartridge>(new Cartridge());
    cartridge_->Load(data, size);
    return cartridge_->is_loaded();
}

void Vectrex::UnloadCartridge()
{
    // Can only unload a cartridge, if one has been loaded
    if (cartridge_ && cartridge_->is_loaded())
        cartridge_->Unload();
}

