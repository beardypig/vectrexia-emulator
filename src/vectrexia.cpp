/*
Copyright (C) 2016 beardypig

This file is part of Vectrexia.

Vectrexia is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Vectrexia is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Vectrexia. If not, see <http://www.gnu.org/licenses/>.
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
    uint64_t cycles_run = 0;
    while (cycles_run < cycles)
    {
        uint64_t cpu_cycles = 0;
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
            via_->Step();
            vector_buffer_.Step(via_->getPortAState(), via_->getPortBState(),
                                via_->getCA2State(), via_->getCB2State());
            UpdateJoystick(via_->getPortAState(), via_->getPortBState());
            psg_->Step(via_->getPortAState(), (uint8_t) ((via_->getPortBState() >> 3) & 1),
                       1, (uint8_t) ((via_->getPortBState() >> 4) & 1));
            this->cycles++;
        }

        cycles_run += cpu_cycles;
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

static uint8_t read_psg_io(intptr_t ref)
{
    return reinterpret_cast<Vectrex*>(ref)->ReadPSGIO();
}

static void store_psg_reg(intptr_t ref, uint8_t data)
{
    reinterpret_cast<Vectrex*>(ref)->StorePSGReg(data);
}

static uint8_t read_via_porta(intptr_t ref)
{
    return reinterpret_cast<Vectrex*>(ref)->ReadPortA();
}

static uint8_t read_via_portb(intptr_t ref)
{
    return reinterpret_cast<Vectrex*>(ref)->ReadPortB();
}

Vectrex::Vectrex() noexcept
{
    cpu_ = std::make_unique<M6809>();
    via_ = std::make_unique<VIA6522>();
    psg_ = std::make_unique<AY38910>();

    // CPU Callbacks
    cpu_->SetReadCallback(read_mem, reinterpret_cast<intptr_t>(this));
    cpu_->SetWriteCallback(write_mem, reinterpret_cast<intptr_t>(this));

    // VIA Callback
    via_->SetPortAReadCallback(read_via_porta, reinterpret_cast<intptr_t>(this));
    via_->SetPortBReadCallback(read_via_portb, reinterpret_cast<intptr_t>(this));

    // PSG callbacks
    psg_->SetIOReadCallback(read_psg_io, reinterpret_cast<intptr_t>(this));
    psg_->SetRegStoreCallback(store_psg_reg, reinterpret_cast<intptr_t>(this));
}

uint8_t Vectrex::Read(uint16_t addr)
{
    // 0000-7FFF: cartridge
    if (addr < 0x8000 && cartridge_) {
        return cartridge_->Read(addr, (uint8_t) (via_->getPortBState() >> 6 & 1));
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
        cartridge_->Write(addr, (uint8_t) (via_->getPortBState() >> 6 & 1));
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


// function pointers for when PORTA/B are read
uint8_t Vectrex::ReadPortA() {
    return psg_port;
}

uint8_t Vectrex::ReadPortB() {
    return joystick_compare;
}

void Vectrex::UpdateJoystick(uint8_t porta, uint8_t portb) {
    // porta is connected to the databus of the sound chip and DAC
    // portb 3+4 and ca1 for sound chip stuff

    // PB0 - S/H
    // PB1 - SEL 0
    // PB2 - SEL 1
    // PB5 - COMPARE (input)
    // PB6 - CART N/C?
    // PB7 - RAMP
    uint8_t select, pot;
    select = (uint8_t) ((portb >> 1) & 0x3);

    // S/H enables the demultiplexor
    // select selects the channel for the comparator
    switch (select) {
        case 0:
            pot = p1_joystick.pot_x;
            break;
        case 1:
            pot = p1_joystick.pot_y;
            break;
        case 2:
            pot = p2_joystick.pot_x;
            break;
        case 3:
            pot = p2_joystick.pot_y;
            break;
        default:
            pot = 0x7f;
    }

    // compare the value on porta to the selected joystick pot value
    joystick_compare = (uint8_t) ((pot > (porta ^ 0x80)) ? 0x20 : 0);
}

void Vectrex::SetPlayerOne(uint8_t x, uint8_t y, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4)
{
    // the sticks are analog and range from 0x00 -> 0xff (left -> right/down -> up)
    p1_joystick.pot_x = x;
    p1_joystick.pot_y = y;

    // the buttons are active low
    p1_joystick.btn_1 = (uint8_t) (b1 ^ 1);
    p1_joystick.btn_2 = (uint8_t) (b2 ^ 1);
    p1_joystick.btn_3 = (uint8_t) (b3 ^ 1);
    p1_joystick.btn_4 = (uint8_t) (b4 ^ 1);
}

void Vectrex::SetPlayerTwo(uint8_t x, uint8_t y, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4)
{
    p2_joystick.pot_x = x;
    p2_joystick.pot_y = y;

    p2_joystick.btn_1 = (uint8_t) (b1 ^ 1);
    p2_joystick.btn_2 = (uint8_t) (b2 ^ 1);
    p2_joystick.btn_3 = (uint8_t) (b3 ^ 1);
    p2_joystick.btn_4 = (uint8_t) (b4 ^ 1);
}

uint8_t Vectrex::ReadPSGIO()
{
    return (p2_joystick.btn_4 << 7) | \
           (p2_joystick.btn_3 << 6) | \
           (p2_joystick.btn_2 << 5) | \
           (p2_joystick.btn_1 << 4) | \
           (p1_joystick.btn_4 << 3) | \
           (p1_joystick.btn_3 << 2) | \
           (p1_joystick.btn_2 << 1) | \
           (p1_joystick.btn_1);
}

void Vectrex::StorePSGReg(uint8_t reg)
{
    psg_port = reg;
}

VectorBuffer *Vectrex::getFramebuffer()
{
    return vector_buffer_.getVectorBuffer();
}

DebugBuffer *Vectrex::getDebugbuffer()
{
    return vector_buffer_.getDebugBuffer();
}

M6809 &Vectrex::GetM6809()
{
    return *cpu_;
}