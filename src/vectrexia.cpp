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
    cpu_->Reset();
}

long Vectrex::Run(long cycles)
{
    int cycles_run = 0;
    while (cycles_run < cycles)
    {
        int cpu_cycles = 0;
        // run one instruction on the CPU
        // The VIA 6522 interrupt line is connected to the M6809 IRQ line
        m6809_error_t rcode = cpu_->Execute(cpu_cycles, (via_->GetIRQ()) ? IRQ : NONE);
        if (rcode != E_SUCCESS)
        {
            auto registers = cpu_->getRegisters();
            if (rcode == E_UNKNOWN_OPCODE)
                message("Unknown opcode at $%04x [$%02x]", registers.PC - 1, Read((uint16_t) (registers.PC - 1)));
            else if (rcode == E_UNKNOWN_OPCODE_PAGE1)
                message("Unknown page 1 opcode at $%04x [$%02x]", registers.PC - 1,
                        Read((uint16_t) (registers.PC - 1)));
            else if (rcode == E_UNKNOWN_OPCODE_PAGE2)
                message("Unknown page 2 opcode at $%04x [$%02x]", registers.PC - 1),
                        Read((uint16_t) (registers.PC - 1));
        }

        // run the VIA for the same number of cycles
        for (int via_cycles = 0; via_cycles < cpu_cycles; via_cycles++)
        {
            via_->Execute();
            this->cycles++;
        }

        cycles_run += cpu_cycles;
        cycles -= cpu_cycles;
    }
    return cycles_run;
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

static void vectrex_peripheral_step(intptr_t ref,
                                    uint8_t porta, uint8_t portb,
                                    uint8_t ca1, uint8_t ca2,
                                    uint8_t cb1, uint8_t cb2)
{
    reinterpret_cast<Vectrex*>(ref)->PeripheralStep(porta, portb, ca1, ca2, cb1, cb2);
}

Vectrex::Vectrex()
{
    cpu_ = std::make_unique<M6809>();
    via_ = std::make_unique<VIA6522>();
    cpu_->SetReadCallback(read_mem, reinterpret_cast<intptr_t>(this));
    cpu_->SetWriteCallback(write_mem, reinterpret_cast<intptr_t>(this));
    via_->SetUpdateCallback(vectrex_peripheral_step, reinterpret_cast<intptr_t>(this));
}

uint8_t Vectrex::Read(uint16_t addr)
{
    //printf("Reading from addr: $%04x\n", addr);
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
            return via_->Read((uint8_t) (addr & 0xf));
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
            via_->Write((uint8_t) (addr & 0xf), data);
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

VIA6522 &Vectrex::GetVIA6522()
{
    return *via_;
}

void Vectrex::PeripheralStep(uint8_t porta, uint8_t portb, uint8_t ca1, uint8_t ca2, uint8_t cb1, uint8_t cb2)
{
    vector_buffer.BeamStep(porta, portb, ca2, cb2);
}

Vectorizer &Vectrex::GetVectorizer()
{
    return vector_buffer;
}
