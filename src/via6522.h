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
#ifndef VECTREXIA_VIA6522_H
#define VECTREXIA_VIA6522_H

#include <stdint.h>

// Registers
enum {
    REG_ORB = 0,
    REG_ORA,
    REG_DDRB,
    REG_DDRA,
    REG_T1CL,
    REG_T1CH,
    REG_T1LL,
    REG_T1LH,
    REG_T2CL,
    REG_T2CH,
    REG_SR,
    REG_ACR,
    REG_PCR,
    REG_IFR,
    REG_IER,
    REG_ORA_NO_HANDSHAKE
};

// Interrupts
enum {
    CA2_INT    = 0x01,
    CA1_INT    = 0x02,
    SR_INT     = 0x04,
    CB2_INT    = 0x08,
    CB1_INT    = 0x10,
    TIMER2_INT = 0x20,
    TIMER1_INT = 0x40,
    IRQ_MASK   = 0x80
};

// ACR flags
enum {
    // Timer 1
    T1_TIMED            = 0x00,
    T1_CONTINUOUS       = 0x40,
    T1_TIMED_PB7        = 0x80,
    T1_CONTINUOUS_PB7   = 0xc0,
    T1_PB7_CONTROL      = T1_TIMED_PB7 & T1_CONTINUOUS_PB7,
    T1_MASK             = T1_CONTINUOUS_PB7,
    // Timer 2
    T2_TIMED            = 0x00,
    T2_PULSE_PB6        = 0x20,
    T2_MASK             = T2_PULSE_PB6,
    // Shift Register
    SR_DISABLED         = 0x00,
    SR_IN_T2            = 0x04,
    SR_IN_O2            = 0x08,
    SR_IN_EXT           = 0x0C,
    SR_OUT_T2_FREE      = 0x10,
    SR_OUT_T2           = 0x14,
    SR_OUT_O2           = 0x18,
    SR_OUT_EXT          = 0x1c,
    SR_MASK             = SR_OUT_EXT,
    SR_EXT              = (SR_IN_EXT & SR_OUT_EXT),
    SR_IN_OUT           = SR_OUT_T2_FREE,   // the direction of the shift (0 in, 1 out)
    // PA/PB Latching
    PA_LATCH_OFF        = 0x00,
    PA_LATCH_ON         = 0x01,
    PA_LATCH_MASK       = PA_LATCH_ON,
    PB_LATCH_OFF        = 0x00,
    PB_LATCH_ON         = 0x02,
    PB_LATCH_MASK       = PB_LATCH_ON
};

// PCR Flags

enum {
    // CB2 Flags
    CB2_INPUT           = 0x00,
    CB2_INPUT_NEG       = 0x00,
    CB2_INPUT_INT_NEG   = 0x20,
    CB2_INPUT_POS       = 0x40,
    CB2_INPUT_INT_POS   = 0x60,
    CB2_OUTPUT          = 0x80,
    CB2_OUT_HANDSHAKE   = 0x80,
    CB2_OUT_PULSE       = 0xa0,
    CB2_OUT_LOW         = 0xc0,
    CB2_OUT_HIGH        = 0xe0,
    CB2_IN_OUT          = CB2_OUT_HANDSHAKE,
    CB2_MASK            = CB2_OUT_HIGH,

    // CB1 Flags
    CB1_INT_NEG         = 0x00,
    CB1_INT_POS         = 0x10,
    CB1_MASK            = CB1_INT_POS,

    // CA2 Flags
    CA2_INPUT           = 0x00,
    CA2_INPUT_NEG       = 0x00,
    CA2_INPUT_INT_NEG   = 0x02,
    CA2_INPUT_POS       = 0x04,
    CA2_INPUT_INT_POS   = 0x06,
    CA2_OUTPUT          = 0x08,
    CA2_OUT_HANDSHAKE   = 0x08,
    CA2_OUT_PULSE       = 0x0a,
    CA2_OUT_LOW         = 0x0c,
    CA2_OUT_HIGH        = 0x0e,
    CA2_IN_OUT          = CA2_OUT_HANDSHAKE,
    CA2_MASK            = CA2_OUT_HIGH,

    // CA1 Flags
    CA1_INT_NEG         = 0x00,
    CA1_INT_POS         = 0x01,
    CA1_MASK            = CA1_INT_POS
};

class VIA6522
{
    using port_callback_t = uint8_t (*)(intptr_t);
    using update_callback_t = void (*)(intptr_t, uint8_t, uint8_t, bool, bool, bool, bool);

    struct Timer
    {
        uint16_t counter;
        bool enabled;
        bool one_shot;  // if the timer in one shot mode has been trigger yet
    };


    struct ShiftRegister
    {
        uint8_t shifted;  // number of bits shifts, set the SR interrupt on 8
        uint8_t counter;  // controlled by timer 2 latch
        bool enabled;
        void update(VIA6522 &via6522, uint8_t edge)
        {
            // a CB1 positive edge caused the data from CB2 is shifted in/out to/from the shift register
            // bit 4 of the ACR controls the direction of the shift.
            if (enabled) {
                if (!via6522.cb1_state_sr && edge) {   // positive edge
                    if ((via6522.registers.ACR & SR_MASK) != SR_OUT_T2_FREE) { // in free run mode, the counter is ignored.
                        shifted++; // increment bit counter
                    }

                    // do not perform any shifting unless the counter is less than 8
                    // in free run mode, the counter is not updated and the shift will continue
                    //via_debug("SR: shifting... (bits: %d) free_run: %d out?: %d = %d\r\n", shifted,
                    //          (via6522.ACR & SR_MASK) == SR_OUT_T2_FREE, via6522.ACR & SR_IN_OUT, via6522.SR >> 7);
                    if (via6522.registers.ACR & SR_IN_OUT) { // out
                        // cb2_state becomes the 7th bit of SR if it's an output
                        //if (via6522.PCR & CB2_IN_OUT)
                        via6522.cb2_state_sr = via6522.registers.SR >> 7;
                        // roll bits around the SR
                        via6522.registers.SR = (via6522.registers.SR << 1) | (via6522.registers.SR >> 7);
                    } else { // in
                        // bits start at bit 0 and are shifted towards bit 7
                        via6522.registers.SR <<= 1;
                        // if CB2 is set to output the shift in 0s, else shift in the CB2 state - in the Vectrex CB2 is
                        // always an output.
                        via6522.registers.SR |= (uint8_t) (via6522.cb2_state) & \
                                                    (via6522.registers.PCR & CB2_IN_OUT) ? 0x01 : 0x00;
                    }

                    if (shifted == 8) {
                        //via_debug("Shifting complete, 8 bits shifted: SR = 0x%02x\r\n", via6522.SR);
                        via6522.set_ifr(SR_INT, 1);
                        // disable the shift register
                        enabled = false;
                    }
                }
                // when disable the last state should be HIGH
                via6522.cb1_state_sr = edge;
            }
        }
    };

    struct Registers
    {
        union
        {
            uint8_t data[0x10];
            struct
            {
                uint8_t ORB,    // port b
                        ORA,    // port a
                        DDRB,
                        DDRA,
                        T1CL,
                        T1CH,
                        T1LL,
                        T1LH,
                        T2CL,   // read-only counter, and write-only latch
                        T2CH,
                        SR,
                        ACR,
                        PCR,
                        IFR,
                        IER;
            };
        };
        // internal input ports, and latched ports
        uint8_t IRA,
                IRB,
                IRA_latch,
                IRB_latch;
        uint8_t PB7;
    } registers;

    Timer timer1;
    Timer timer2;

    ShiftRegister sr;

    uint8_t ca1_state, ca2_state;
    uint8_t cb1_state, cb2_state, cb1_state_sr, cb2_state_sr;

    uint64_t clk;

    // port a/b read callbacks
    port_callback_t porta_callback_func = nullptr;
    intptr_t        porta_callback_ref = 0;
    port_callback_t portb_callback_func = nullptr;
    intptr_t        portb_callback_ref = 0;

    // execute update callback
    update_callback_t update_callback_func = nullptr;
    intptr_t          update_callback_ref = 0;

    // Update the state of the IFR
    inline void update_ifr(void) {
        //via_debug("Updating IFR: IER=0x%02x, IFR=0x%02x\r\n", registers.IER, registers.IFR);
        // test if any enabled interrupts are set in the interrupt flag register
        if ((registers.IFR & ~IRQ_MASK) & (registers.IER & ~IRQ_MASK)) {
            // set the MSB high if there is an active interrupt
            registers.IFR |= IRQ_MASK;
        } else {
            // if there are no active interrupts then clear the MSB
            registers.IFR &= ~IRQ_MASK;
        }
    }

    // Set or clear a bit in the IFR. If an interrupt is enabled and set then the MSB of IFR is set.
    inline void set_ifr(uint8_t bits, uint8_t state) {
        if (state) {
            // set the bit in the IFR
            registers.IFR |= bits & ~IRQ_MASK;
        } else {
            // clear the bits
            registers.IFR &= ~(bits & ~IRQ_MASK);
        }
        update_ifr();
    }

    // Set or clear a bit in the IER.
    inline void set_ier(uint8_t bits, uint8_t state) {
        if (state) {
            // set the bits in the IER
            registers.IER |= bits & ~IRQ_MASK;
        } else {
            // clear the bits
            registers.IER &= ~(bits & ~IRQ_MASK);
        }
        update_ifr();
    }

    inline uint8_t read_porta()
    {
        // mask the output data
        uint8_t ora = registers.ORA & registers.DDRA;
        // if PORTA is in latching mode
        if (registers.ACR & PA_LATCH_MASK) {
            //via_debug("Reading PORTA in latched mode\r\n");
            ora |= registers.IRA_latch & ~registers.DDRA;
        } else {
            ora |= (porta_callback_func) ? porta_callback_func(porta_callback_ref) & ~registers.DDRA : 0;
        }
        return ora;
    }

    inline uint8_t read_portb()
    {
        uint8_t orb = registers.ORB;
        // if Timer 1 has control of PB7
        if (registers.ACR & T1_PB7_CONTROL) {
            // mask PB7 from ORB and use the Timer 1 controlled PB7
            orb = (uint8_t) ((orb & ~0x80) | registers.PB7);
        }
        // clear pins that are marked as input
        orb &= registers.DDRB;

        // if PORTB is in latching mode
        if (registers.ACR & PB_LATCH_MASK) {
            //via_debug("Reading PORTB in latched mode\r\n");
            orb |= registers.IRB_latch & ~registers.DDRB;
        } else {
            orb |= (portb_callback_func) ? portb_callback_func(portb_callback_ref) & ~registers.DDRB : 0;
        }
        return orb;
    }

public:

    VIA6522() = default;
    void Step();
    void Reset();

    // Set callbacks for read and write, must be a static function
    void SetPortAReadCallback(port_callback_t func, intptr_t ref);
    void SetPortBReadCallback(port_callback_t func, intptr_t ref);
    void SetUpdateCallback(update_callback_t func, intptr_t ref);

    uint8_t Read(uint8_t reg);              // read from VIA register
    void Write(uint8_t reg, uint8_t data);  // write to VIA register

    uint8_t GetIRQ();

    uint8_t getPortAState();
    uint8_t getPortBState();
    uint8_t getCA1State() { return ca1_state; }
    uint8_t getCA2State() { return ca2_state; }
    uint8_t getCB1State() { return (registers.ACR & SR_EXT) == SR_EXT ? cb1_state : cb1_state_sr; }
    uint8_t getCB2State() { return (registers.ACR & SR_IN_OUT) ? cb2_state_sr : cb2_state; };
};

#endif //VECTREXIA_VIA6522_H
