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

#include <stdint.h>
#include <via6522.h>
#include <stddef.h>
#include <stdio.h>
#include "via6522.h"

uint8_t VIA6522::Read(uint8_t reg)
{
    uint8_t data = 0;

    switch (reg & 0xf) {
        case REG_ORB:
            data = read_portb();
            break;

        case REG_ORA:
            if ((registers.PCR & CA2_MASK) == CA2_OUTPUT) {
                // CA2 goes low to signal "data taken", in pulse mode that will be restored to 1 at the end of the
                // VIA emulation
                ca2_state = 0;
            }
        case REG_ORA_NO_HANDSHAKE:
            data = read_porta();
            break;

            // Timer 1
        case REG_T1CL:  // timer 1 low-order counter
            // stop timer 1
            timer1.enabled = false;

            // Set PB7 if Timer 1 has control of PB7
            if (registers.ACR & T1_PB7_CONTROL)
                registers.PB7 = 0x80;

            // clear the timer 1 interrupt
            set_ifr(TIMER1_INT, 0);

            data = (uint8_t)(timer1.counter & 0xff);
            break;
        case REG_T1CH:  // timer 1 high-order counter
            data = (uint8_t)(timer1.counter >> 8);
            break;
        case REG_T1LL:  // timer 1 low-order latch
            data = registers.T1LL;
            break;
        case REG_T1LH:  // timer 2 high-order latch
            data = registers.T1LH;
            break;

            // Timer 2
        case REG_T2CL:  // timer 2 low-order counter
            // stop timer 2
            timer2.enabled = false;

            // clear the timer 2 interrupt
            set_ifr(TIMER2_INT, 0);

            data = (uint8_t)(timer2.counter & 0xff);
            break;
        case REG_T2CH:  // timer 2 high-order counter
            data = (uint8_t)(timer2.counter >> 8);
            break;

            // Shift Register
        case REG_SR:
            // clear the SR interrupt
            set_ifr(SR_INT, 0);
            // reset shifted bits counter
            sr.shifted = 0;
            sr.enabled = true;
            data = registers.SR;
            break;

            // Interrupt Registers
        case REG_IER:
            // interrupt enable register, the MSB is always set when reading.
            data = registers.IER | IRQ_MASK;
            break;

            // Basic Reads
        case REG_DDRB:
            // DDR register 0 for output, 1 for input
            data = registers.DDRB;
            break;
        case REG_DDRA:
            data = registers.DDRA;
            break;
        case REG_ACR:
            data = registers.ACR;
            break;
        case REG_PCR:
            data = registers.PCR;
            break;
        case REG_IFR:
            data = registers.IFR;
            break;
        default:
            break;
    }
    // invalid address
    return data;
}

void VIA6522::Write(uint8_t reg, uint8_t data)
{
    switch (reg & 0xf) {

        case REG_ORB:
            // Port B lines (CB1, CB2) handshake on a write operation only.
            if ((registers.PCR & CB2_MASK) == CB2_OUTPUT) {
                cb2_state = 0;
            }

            registers.ORB = data;
            break;
        case REG_ORA:
            // If CA2 is output
            if ((registers.PCR & CA2_MASK) == CA2_OUTPUT) {
                ca2_state = 1;
            }
        case REG_ORA_NO_HANDSHAKE:
            registers.ORA = data;
            break;

            // Timer 1
        case REG_T1CL:  // timer 1 low-order counter
            registers.T1LL = data;
            break;
        case REG_T1CH:  // timer 1 high-order counter
            registers.T1LH = data;
            timer1.counter = (registers.T1LH << 8) | registers.T1LL;

            // start timer 1
            timer1.enabled = true;
            timer1.one_shot = false;
            // Does Timer 1 control PB7
            if ((registers.ACR & T1_MASK) == T1_PB7_CONTROL)
                // set timer 1's registers.PB7 state to low
                registers.PB7 = 0;

            // clear the timer 1 interrupt
            set_ifr(TIMER1_INT, 0);
            break;
        case REG_T1LL:  // timer 1 low-order latch
            registers.T1LL = data;
            break;
        case REG_T1LH:  // timer 2 high-order latch
            registers.T1LH = data;
            break;

            // Timer 2
        case REG_T2CL:  // timer 2 low-order counter
            registers.T2CL = data;
            break;
        case REG_T2CH:  // timer 2 high-order counter
            registers.T2CH = data;
            timer2.counter = (registers.T2CH << 8) | registers.T1CL;

            // start timer 2
            timer2.enabled = true;
            timer2.one_shot = false;

            // clear the timer 2 interrupt
            set_ifr(TIMER2_INT, 0);
            break;

            // Shift Register
        case REG_SR:
            // clear the SR interrupt
            set_ifr(SR_INT, 0);
            // reset shifted bits counter
            sr.shifted = 0;
            registers.SR = data;
            sr.enabled = true;
            break;

            // Interrupt Registers
        case REG_IFR:
            // interrupt flag register
            set_ifr(data, 0);
            break;

        case REG_IER:
            // interrupt enable register
            // enable or disable interrupts based on the MSB and using the rest of the data as a mask
            set_ier(data & ~IRQ_MASK, (uint8_t) (data & 0x80 ? 1 : 0));
            break;

        case REG_PCR:
            registers.PCR = data;
            // if CA/B2 is in OUT LOW mode, set to low, otherwise high
            ca2_state = (uint8_t) (((registers.PCR & CA2_MASK) == CA2_OUT_LOW) ? 0 : 1);
            cb2_state = (uint8_t) (((registers.PCR & CB2_MASK) == CB2_OUT_LOW) ? 0 : 1);
            break;

            // Basic Writes
        case REG_DDRB:
            registers.DDRB = data;
            break;
        case REG_DDRA:
            registers.DDRA = data;
            break;
        case REG_ACR:
            registers.ACR = data;
            break;
        default:
            break;
    }
}

void VIA6522::Reset()
{
    // registers
    registers.ORB = 0;
    registers.ORA = 0;
    registers.DDRB = 0;
    registers.DDRA = 0;
    registers.T1CL = 0;
    registers.T1CH = 0;
    registers.T1LL = 0;
    registers.T1LH = 0;
    registers.T2CL = 0;
    registers.T2CH = 0;
    registers.SR = 0;
    registers.ACR = 0;
    registers.PCR = 0;
    registers.IFR = 0;
    registers.IER = 0;
    registers.IRA = 0;
    registers.IRB = 0;
    registers.IRA_latch = 0;
    registers.IRB_latch = 0;

    ca1_state = 0;
    ca2_state = 1;
    cb1_state = 0;
    cb1_state_sr = 0;
    cb2_state = 1;
    cb2_state_sr = 0;

    // timer data
    timer1.counter = 0;
    timer1.enabled = false;
    timer1.one_shot = false;
    timer2.counter = 0;
    timer2.enabled = false;
    timer2.one_shot = false;
    registers.PB7 = 0x80;

    // shift register
    sr.enabled = false;
    sr.shifted = 0;
    sr.counter = 0;

    clk = 0;
}

void VIA6522::Step()
{
    // Update any delayed signals
    delayed_signals.tick(clk++);

    // Timers
    if (timer1.enabled) {
        // decrement the counter and test if it has rolled over
        if (--timer1.counter == 0xffff) {
            // is the continuous interrupt bit set
            if (registers.ACR & T1_CONTINUOUS) {
                // set the timer 1 interrupt
                set_ifr(TIMER1_INT, 1);

                // toggle PB7 (ACR & 0x80)
                if (registers.ACR & T1_PB7_CONTROL) {
                    registers.PB7 ^= 0x80;
                }
                //via_debug_timer("\r\n");

                // reload counter from latches
                timer1.counter = (registers.T1LH << 8) | registers.T1LL;
            } else { // one-shot mode
                if (!timer1.one_shot) {
                    // set timer 1 interrupt
                    set_ifr(TIMER1_INT, 1);

                    if (registers.ACR & T1_PB7_CONTROL) {
                        //via_debug_timer2(", PB7 set");
                        // restore PB7 to 1, it was set to 0 by writing to T1C-H
                        registers.PB7 = 0x80;
                    }
                    //via_debug_timer2("\r\n");
                    timer1.one_shot = true;
                }
            }
        }
    }

    if (timer2.enabled) {
        // pulsed mode is not used
        if ((registers.ACR & T2_MASK) == T2_TIMED) { // timed, one-shot mode
            // In one-shot mode the timer keeps going, but the interrupt is only triggered once
            if (--timer2.counter == 0xffff && !timer2.one_shot) {
                // set the Timer 2 interrupt
                set_ifr(TIMER2_INT, 1);
                timer2.one_shot = true;
            }
        }
    }

    switch (registers.ACR & SR_MASK) {
        case SR_DISABLED:
        case SR_OUT_EXT:
        case SR_IN_EXT:
            // This mode is controlled by CB1 positive edge, however in the Vectrex CB1 is not connected.
            // For these modes CB1 is an input
            break;
        case SR_IN_T2:
        case SR_OUT_T2:
        case SR_OUT_T2_FREE:
            // CB1 becomes an output
            // when counter T2 counter rolls
            if (sr.counter == 0x00) {
                // Toggle CB1 on the clock time out
                sr.update(*this, (uint8_t) (cb1_state_sr ^ 1));
            }
            break;
        case SR_IN_O2:
        case SR_OUT_O2:
            // CB1 is an output
            // Toggle CB1 on the phase 2 clock
            sr.update(*this, (uint8_t) (cb1_state_sr ^ 1));
        default:break;
    }

    if (--sr.counter == 0xff) {
        // reset the Shift Reigster T2 counter to T2 low-order byte
        sr.counter = registers.T2CL;
    }

    // End of pulse mode handshake
    // If PORTA is using pulse mode handshaking, restore CA2 to 1 at the beginning of the next cycle
    if ((registers.PCR & CA2_MASK) == CA2_OUT_PULSE)
        delayed_signals.enqueue(clk+1, &ca2_state, 1);

    // Same for PORTB
    if ((registers.PCR & CB2_MASK) == CB2_OUT_PULSE)
        delayed_signals.enqueue(clk+1, &ca2_state, 1);


}

void VIA6522::SetPortAReadCallback(VIA6522::port_callback_t func, intptr_t ref)
{
    porta_callback_func = func;
    porta_callback_ref = ref;
}

void VIA6522::SetPortBReadCallback(VIA6522::port_callback_t func, intptr_t ref)
{
    portb_callback_func = func;
    portb_callback_ref = ref;
}

uint8_t VIA6522::GetIRQ()
{
    return registers.IFR & IRQ_MASK;
}

// returns the port b data bus state
uint8_t VIA6522::getPortBState()
{
    uint8_t portb;
    // if Timer 1 has control of PB7
    if (registers.ACR & T1_PB7_CONTROL) {
        // mask PB7 from ORB and use the Timer 1 controlled PB7
        portb = (uint8_t) ((registers.ORB & ~0x80) | registers.PB7);
    } else {
        portb = registers.ORB;
    }
    return portb;
}

// returns the port a data bus state
uint8_t VIA6522::getPortAState()
{
    return registers.ORA;
}
