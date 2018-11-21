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
    cpu->Reset();
}

uint64_t Vectrex::Run(uint64_t cycles)
{
    uint64_t cycles_run = 0;
    while (cycles_run < cycles)
    {
        uint64_t cpu_cycles = 0;
        // run one instruction on the CPU
        // The VIA 6522 interrupt line is connected to the M6809 IRQ line
        m6809_error_t rcode = cpu->Execute(cpu_cycles, (via->GetIRQ()) ? IRQ : NONE);
        if (rcode != E_SUCCESS)
        {
            auto registers = cpu->getRegisters();
            if (rcode == E_UNKNOWN_OPCODE)
                message("Unknown opcode at $%04x [$%02x]", registers.PC - 1, Read((uint16_t) (registers.PC - 1)));
            else if (rcode == E_UNKNOWN_OPCODE_PAGE1)
                message("Unknown page 1 opcode at $%04x [$%02x]", registers.PC - 1,
                        Read((uint16_t) (registers.PC - 1)));
            else if (rcode == E_UNKNOWN_OPCODE_PAGE2)
                message("Unknown page 2 opcode at $%04x [$%02x]", registers.PC - 1),
                        Read((uint16_t) (registers.PC - 1));
        }

        // run the various circuits for the same amount of cycles and in the correct order
        for (int via_cycles = 0; via_cycles < cpu_cycles; via_cycles++)
        {
            via->step();
            psg->step();
            dac->step();
            mpx->step();
            joy->step();
            vec->step();
            this->cycles++;
        }

        cycles_run += cpu_cycles;
    }
    return cycles_run;
}

bool Vectrex::LoadCartridge(const uint8_t *data, size_t size)
{
    cartridge = std::make_unique<Cartridge>();
    cartridge->Load(data, size);
    return cartridge->is_loaded();
}

void Vectrex::UnloadCartridge()
{
    // Can only unload a cartridge, if one has been loaded
    if (cartridge && cartridge->is_loaded())
        cartridge->Unload();
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
    cpu = std::make_unique<M6809>    ();
    joy = std::make_unique<Joystick> ();
    via = std::make_unique<VIA6522>  ();
    psg = std::make_unique<AY38910>  (via.get()->ports(), joy.get()->ports());
    dac = std::make_unique<MC1408P8> (psg.get()->ports());
    mpx = std::make_unique<CD4052B>  (via.get()->ports(), dac.get(), joy.get()->ports());
    vec = std::make_unique<SWVectorizer>(via.get()->ports(), mpx.get()->ports(), dac.get());

    // CPU Callbacks
    cpu->SetReadCallback(read_mem, reinterpret_cast<intptr_t>(this));
    cpu->SetWriteCallback(write_mem, reinterpret_cast<intptr_t>(this));
}

uint8_t Vectrex::Read(uint16_t addr)
{
    // 0000-7FFF: cartridge
    if (addr < 0x8000 && cartridge) {
        return cartridge->Read(addr, (uint8_t) (via->getPortBState() >> 6 & 1));
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
            return via->Read((uint8_t) (addr & 0xf));
        }
    }
    return 0x00;
}

void Vectrex::Write(uint16_t addr, uint8_t data)
{
    // 0000-7FFF: cartridge
    if (addr < 0x8000 && cartridge) {
        cartridge->Write(addr, (uint8_t) (via->getPortBState() >> 6 & 1));
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
            via->Write((uint8_t) (addr & 0xf), data);
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

void Vectrex::SetPlayerOne(uint8_t x, uint8_t y, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4)
{
    // the sticks are analog and range from 0x00 -> 0xff (left -> right/down -> up)
    joy->p1_joystick.pot_x = x;
    joy->p1_joystick.pot_y = y;

    // the buttons are active low
    joy->p1_joystick.btn_1 = (uint8_t) (b1 ^ 1);
    joy->p1_joystick.btn_2 = (uint8_t) (b2 ^ 1);
    joy->p1_joystick.btn_3 = (uint8_t) (b3 ^ 1);
    joy->p1_joystick.btn_4 = (uint8_t) (b4 ^ 1);
}

void Vectrex::SetPlayerTwo(uint8_t x, uint8_t y, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4)
{
    joy->p2_joystick.pot_x = x;
    joy->p2_joystick.pot_y = y;
    joy->p2_joystick.btn_1 = (uint8_t) (b1 ^ 1);
    joy->p2_joystick.btn_2 = (uint8_t) (b2 ^ 1);
    joy->p2_joystick.btn_3 = (uint8_t) (b3 ^ 1);
    joy->p2_joystick.btn_4 = (uint8_t) (b4 ^ 1);
}

framebuffer_t *Vectrex::getFramebuffer()
{
    return vec->getFrameBuffer();
}

M6809 &Vectrex::GetM6809()
{
    return *cpu;
}