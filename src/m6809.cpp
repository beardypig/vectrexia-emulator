#include <memory>
#include "m6809.h"


M6809::M6809(const read_callback_t &read_callback,
             const write_callback_t &write_callback)
        : read_callback(read_callback), write_callback(write_callback)
{
    opcode_handlers[0x3A] = std::addressof(opcodewrap<op_abx_inherent>);

    opcode_handlers[0x8B] = std::addressof(opcodewrap<op_adda_immediate>);
    opcode_handlers[0x9B] = std::addressof(opcodewrap<op_adda_direct>);
    opcode_handlers[0xBB] = std::addressof(opcodewrap<op_adda_extended>);

    opcode_handlers[0xC3] = std::addressof(opcodewrap<op_addd_immediate>);

    opcode_handlers[0x80] = std::addressof(opcodewrap<op_suba_immediate>);
    opcode_handlers[0x85] = std::addressof(opcodewrap<op_bita_immediate>);
    Reset();
}

void M6809::Reset()
{
    registers.A = 0;
    registers.B = 0;
    registers.X = 0;
    registers.Y = 0;
    registers.USP = 0;
    registers.SP = 0;
    registers.DP = 0;
    registers.CC = 0;

    // reset sets the PC to the reset vector found at $FFFE
    registers.PC = Read16(RESET_VECTOR);
}

uint8_t M6809::Read8(uint16_t addr)
{
    return read_callback(addr);
}

uint16_t M6809::Read16(uint16_t addr)
{
    return (uint16_t) Read8(addr) | (uint16_t) Read8((uint16_t) (addr + 1)) << 8;
}

void M6809::Write8(uint16_t addr, uint8_t data)
{
    write_callback(addr, data);
}

void M6809::Write16(uint16_t addr, uint16_t data)
{
    Write8(addr, (uint8_t) (data & 0xff));
    Write8((uint16_t) (addr + 1), (uint8_t) (data >> 8));
}

uint8_t M6809::ReadPC8()
{
    return Read8(registers.PC++);
}

uint16_t M6809::ReadPC16()
{
    uint16_t bytes = Read16(registers.PC);
    registers.PC += 2;
    return bytes;
}

uint8_t M6809::NextOpcode()
{
    return ReadPC8();
}


m6809_error_t M6809::Execute(int &cycles)
{
    uint8_t opcode = NextOpcode();
    cycles += 1;

    if (opcode == 0x10)
    {
        opcode = NextOpcode();
        cycles += 1;
        switch (opcode)
        {
            default:
                return E_UNKNOWN_OPCODE_PAGE1;
        }
    }
    else if (opcode == 0x11)
    {
        opcode = NextOpcode();
        cycles += 1;
        switch (opcode)
        {
            default:
                return E_UNKNOWN_OPCODE_PAGE2;
        }
    }
    else
    {
        opcode_handler_t opcode_handler = opcode_handlers[opcode];
        //op_adda_immediate(*this, cycles);
        // op_dec_direct(*this, cycles);
        if (opcode_handler) {
            opcode_handler(*this, cycles);
            return E_SUCCESS;
        }
        else {
            printf("Unknown opcode 0x%02x\n", opcode);
            //std::cout << "Uknown opcode" << std::hex << opcode << std::endl;
            return E_UNKNOWN_OPCODE;
        }
    }

    return E_SUCCESS;
}
