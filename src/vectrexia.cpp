/*
Copyright (C) 2016 beardypig

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
    cpu_->Reset();
}

uint64_t Vectrex::Run(uint64_t cycles)
{
    uint64_t cpu_cycles = 0;
    m6809_error_t rcode = cpu_->Execute(cpu_cycles);

    if (rcode != E_SUCCESS)
    {
        auto registers = cpu_->getRegisters();
        if (rcode == E_UNKNOWN_OPCODE)
            message("Unknown opcode at $%04x [$%02x]", registers.PC-1, Read((uint16_t) (registers.PC - 1)));
        else if (rcode == E_UNKNOWN_OPCODE_PAGE1)
            message("Unknown page 1 opcode at $%04x [$%02x]", registers.PC-1, Read((uint16_t) (registers.PC - 1)));
        else if (rcode == E_UNKNOWN_OPCODE_PAGE2)
            message("Unknown page 2 opcode at $%04x [$%02x]", registers.PC-1), Read((uint16_t) (registers.PC - 1));
    }

    return cpu_cycles;
}

bool Vectrex::LoadCartridge(const uint8_t *data, size_t size)
{
    cartridge_ = std::make_unique<Cartridge>();
    cartridge_->Load(data, size);
    return cartridge_->is_loaded();
}

void Vectrex::UnloadCartridge()
{
    // Can only unload a cartridge, if one has been loaded
    if (cartridge_ && cartridge_->is_loaded())
        cartridge_->Unload();
}


static uint8_t read_mem(intptr_t ref, uint16_t addr)
{
    return reinterpret_cast<Vectrex*>(ref)->Read(addr);
}

static void write_mem(intptr_t ref, uint16_t addr, uint8_t data)
{
    reinterpret_cast<Vectrex*>(ref)->Write(addr, data);
}

Vectrex::Vectrex() noexcept
{
    cpu_ = std::make_unique<M6809>();
    cpu_->SetReadCallback(read_mem, reinterpret_cast<intptr_t>(this));
    cpu_->SetWriteCallback(write_mem, reinterpret_cast<intptr_t>(this));
}

uint8_t Vectrex::Read(uint16_t addr)
{
    // 0000-7FFF: cartridge
    if (addr < 0x8000 && cartridge_) {
        return cartridge_->Read(addr);
    }
    // E000-FFFF: system ROM
    else if (addr >= 0xe000) {
        // offset the addr relative to E000
        return sysrom_[addr & ~0xe000];
    }
    // 8000-C7FF: Unused
    // C800-CFFF: RAM
    // D000-D7FF: 6522VIA
    // D800-DFFF: don't use for reads
    else if ((addr >= 0xc800)) {
        // C800-CF77: RAM
        if (addr < 0xD000) {
            return ram_[addr & 0x3ff];
        }
        else if (addr < 0xD800) {
            // D000-D7FF: 6522VIA I/O
        }
    }
    return 0x00;
}

void Vectrex::Write(uint16_t addr, uint8_t data)
{
    // 0000-7FFF: cartridge
    if (addr < 0x8000 && cartridge_) {
        cartridge_->Write(addr, data);
    }
    // E000-FFFF: system ROM
    else if (addr >= 0xe000) {
        // system rom - cannot write
    }
    // 8000-C7FF: Unused
    // C800-CFFF: RAM
    // D000-D7FF: 6522VIA
    // D800-DFFF: For writes can write to _both_ RAM and 6522
    else if (addr >= 0xc800) {
        // C800-CF77: RAM
        if (addr & 0x800) {
            ram_[addr & 0x3ff] = data;
        }
        if (addr & 0x1000) {
            // D000-D7FF: 6522VIA I/O
        }
    }
}

void Vectrex::message(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[vectrexia]: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

M6809 &Vectrex::GetM6809()
{
    return *cpu_;
}
