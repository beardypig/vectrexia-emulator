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
#include <memory>
#include <algorithm>
#include <sstream>
#include <regex>
#include <iterator>
#include "vectorizer.h"
#include "debugger.h"

static uint8_t read_mem(intptr_t ref, uint16_t addr)
{
    return reinterpret_cast<Vectrex*>(ref)->Read(addr);
}

Debugger::Debugger() : prev_pc(0)
{

    initscr();
    noecho();
    nodelay(stdscr, TRUE);
    curs_set(capture);
    keypad(stdscr, TRUE);
    start_color();

    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);

    Refresh();

    getmaxyx(stdscr, this->height, this->width);

    // set up the vectrex
    disasm.SetReadCallback(read_mem, reinterpret_cast<intptr_t>(&vectrex));
    Reset();

    // start the gui
    program_win = std::make_unique<Window>("Program", height - 3, 34, 1, 1);
    register_win = std::make_unique<Window>("Registers", 10, 41, 13, 36);
    stack_win = std::make_unique<Window>("Stack", 12, 20, 1, 36);
    user_stack_win = std::make_unique<Window>("User Stack", 12, 20, 1, 57);
    via_win = std::make_unique<Window>("VIA 6522", 15, 30, 1, 78);
    vect_win = std::make_unique<Window>("Vector Beam", 7, 30, 16, 78);

    command_win = std::make_unique<Window>("", 2, width - 2, height - 2, 1, false);
    help_win = std::make_unique<Window>("Help", height - 2, width - 2, 1, 1);

    UpdateCommandLine("");

    instruction_log.resize(static_cast<size_t>(((height - 3) / 2) - 1));

}

void Debugger::Reset()
{
    vectrex.Reset();
}

void Debugger::UpdatedRegisters()
{
    auto cpu = vectrex.GetM6809();
    auto registers = cpu.getRegisters();

    register_win->Clear();
    register_win->mvprint(2, 2,  "A   : %02x  \'%c\'", registers.A, (isprint(registers.A)) ? registers.A : '.');
    register_win->mvprint(3, 2,  "B   : %02x  \'%c\'", registers.B, (isprint(registers.B)) ? registers.B : '.');
    register_win->mvprint(4, 2,  "D   : %04x", registers.D);
    register_win->mvprint(5, 2,  "X   : %04x", registers.X);
    register_win->mvprint(6, 2,  "Y   : %04x", registers.Y);
    register_win->mvprint(2, 20, "SP  : %04x", registers.SP);
    register_win->mvprint(3, 20, "USP : %04x", registers.USP);
    register_win->mvprint(4, 20, "DP  : %02x", registers.DP);
    register_win->mvprint(5, 20, "PC  : %04x", registers.PC);
    register_win->mvprint(6, 20, "clk : %04d", clk);
    register_win->mvprint(7, 7,  "CC   E F H I N Z V C");
    register_win->mvprint(8, 7,  "%02x = %d %d %d %d %d %d %d %d", registers.CC, registers.flags.E, registers.flags.F, registers.flags.H, registers.flags.I, registers.flags.N, registers.flags.Z, registers.flags.V, registers.flags.C);

    register_win->Refresh();
}

void Debugger::UpdateProgramView()
{
    auto cpu = vectrex.GetM6809();
    auto registers = cpu.getRegisters();

    program_win->Clear();
    uint16_t dis_addr = registers.PC;

    int log_i = 2;

    for (const std::string &ilog : instruction_log)
    {
       program_win->mvprint(log_i++, 2, " %s", ilog.c_str());
    }

    for (int i = log_i; i < height - 4; i++)
    {
        auto is_breakpoint = breakpoints.count(dis_addr);
        std::string disasm_instr = disasm.disasm(dis_addr);
        if (i == log_i)
            // next instruction
            program_win->mvprint_attr(i, 2, COLOR_PAIR(2), ">%s", disasm_instr.c_str());
        else
            // These instruction may run after
            program_win->mvprint(i, 3, disasm_instr.c_str());

        if (is_breakpoint)
            program_win->mvprint_attr(i, 2, COLOR_PAIR(1), "%c", ACS_BLOCK);
    }
    program_win->Refresh();
}

void Debugger::Refresh()
{
    refresh();
}

void Debugger::Run()
{
    auto cpu = vectrex.GetM6809();
    auto registers = cpu.getRegisters();

    if (mode == NORMAL)
    {

        UpdatedRegisters();
        UpdateProgramView();
        UpdateStackViews();
        UpdateVIAView();

        // run for at least one cycle
        if (state == RUN || state == STEP || state == MEGA_STEP)
        {
            uint16_t dis_addr = registers.PC;
            std::string disasm_instr = disasm.disasm(dis_addr);

            clk += vectrex.Run((state == MEGA_STEP) ? 30000 : 1);

            // The PC might not change if the CPU is waiting for an interrupt
            registers = vectrex.GetM6809().getRegisters();
            if (prev_pc != registers.PC)
            {
                // This is the instruction that will run next
                if (instruction_log.size() == INSTRUCTION_LOG_SIZE)
                    instruction_log.pop_front();
                instruction_log.push_back(disasm_instr);
            }

            if (breakpoints.count(registers.PC) || temp_breakpoints.count(registers.PC))
            {
                temp_breakpoints.erase(registers.PC);
                state = HALT;
            }
        }

        // stop on step
        if (state == STEP || state == MEGA_STEP)
            state = HALT;

        prev_pc = registers.PC;
    }

    else if (mode == HELP)
    {
        program_win->hide();
        register_win->hide();
        stack_win->hide();
        user_stack_win->hide();
        via_win->hide();

        ShowHelpWindow();
    }

    Refresh();
}

void Debugger::SetMode(debug_mode_t mode)
{
    state = mode;
}

Debugger::~Debugger()
{
    register_win = nullptr;
    program_win = nullptr;
    stack_win = nullptr;
    user_stack_win = nullptr;
    command_win = nullptr;
    help_win = nullptr;
    via_win = nullptr;
    endwin();
}

void Debugger::UpdateStackViews()
{
    auto cpu = vectrex.GetM6809();
    auto registers = cpu.getRegisters();

    UpdateStackView(registers.SP, stack_win.get());
    UpdateStackView(registers.USP, user_stack_win.get());
}

void Debugger::UpdateStackView(uint16_t sp, Window *window)
{

    window->Clear();

    int sp_i = window->getHeight() - 2;

    while (sp_i > 1)
    {
        auto v = vectrex.Read(sp);
        window->mvprint(sp_i--, 2,  "%04x : %02x  '%c'", sp, v, isprint(v) ? v : '.');
        sp++;
    }

    window->Refresh();
}

void Debugger::UpdateCommandLine(std::string command, std::string result)
{
    std::string pcommand;
    command_win->Clear();

    if (command.length() > width - 5)
    {
        pcommand = command.substr(command.length() - (width - 5));
    }
    else
    {
        pcommand = command;
    }

    command_win->mvprint(0, 0, "$ %s", pcommand.c_str());
    if (result.length())
        command_win->mvprint(1, 0, " -> %s", result.c_str());
    //command_win->move_cursor(0, pcommand.length() + 2);

    command_win->Refresh();
}

void Debugger::CaptureKey(int ch)
{
    if (ch >= 0)
    {
        if (capture)
        {
            if (ch == KEY_ENTER || ch == '\n')
            {
                capture = false;
                RunCommand();
            }
            else if (ch == KEY_BACKSPACE)
            {
                if (command.length())
                    command.pop_back();
            }
            else if (ch == KEY_UP)
            {
                if (command_history_i == command_history.size())
                    commandt = command;
                if (command_history.size() && command_history_i > 0)
                    command = command_history[--command_history_i];
                // limit to 0
                command_history_i = std::max(0, command_history_i);
            }
            else if (ch == KEY_DOWN)
            {
                if (command_history.size() && (command_history_i < static_cast<int>(command_history.size())))
                {
                    if (++command_history_i == command_history.size())
                        command = commandt;
                    else
                        command = command_history[command_history_i];

                    // limit to size-1
                    command_history_i = std::min(static_cast<int>(command_history.size()-1), command_history_i);
                }
            }
            else if (isprint(ch))
            {
                command += ch;
            }
        }
        else
        {
            if (ch == KEY_F(1) || ch == 'q')
                ended = true;
            else
            if (mode == NORMAL)
            {
                if (ch == 'r')
                    state = RUN;
                else if (ch == 's' || ch == ' ' || ch == KEY_ENTER)
                    state = STEP;
                else if (ch == 'h')
                    state = HALT;
                else if (ch == 'm')
                    state = MEGA_STEP;
                else if (ch == '.')
                {
                    // start command mode
                    command = '.';
                    command_history_i = static_cast<int>(command_history.size());
                    capture = true;
                }
            }
            else if (mode == HELP)
            {
                if (ch == 'x') {
                    mode = NORMAL;
                    help_win->hide();
                }
            }
        }

        if (capture)
        {
            if (command.length() == 0)
                capture = false;
            UpdateCommandLine(command);
        }
        curs_set(capture);
    }
}

bool Debugger::is_ended()
{
    return ended;
}

bool Debugger::RunCommand()
{
    command_history.push_back(command);

    std::string command_c = std::regex_replace(command.substr(1), std::regex(" +"), " ");
    command_c = std::regex_replace(command_c, std::regex(" +$"), "");

    std::vector<std::string> tokens;
    std::stringstream ss(command_c);
    std::string item;
    std::string result;

    while (std::getline(ss, item, ' ')) {
        tokens.push_back(item);
    }

    if (tokens.size() > 0)
    {
        if (tokens[0].compare("help") == 0)
        {
            mode = HELP;
        }
        else if (tokens[0].compare("bp") == 0)
        {
            try
            {
                for (int i = 1; i < tokens.size(); i++)
                {
                    breakpoints.insert((unsigned short &&) parse_addr(tokens[i]));
                }
                result = "break point set";
            }
            catch (...)
            {
                result = "invalid argument";
            }
        }
        else if (tokens[0].compare("mem") == 0 && tokens.size() == 2)
        {
            try
            {
                result = str_format("%02x", vectrex.Read((uint16_t) parse_addr(tokens[1])));
            }
            catch (...)
            {
                result = "invalid argument";
            }
        }
        else if (tokens[0].compare("mem16") == 0 && tokens.size() == 2)
        {
            try
            {
                uint16_t addr = (uint16_t) parse_addr(tokens[1]);
                uint16_t data = (uint16_t) vectrex.Read((uint16_t) (addr + 1)) | (uint16_t) vectrex.Read((uint16_t) (addr)) << 8;
                result = str_format("%04x", data);
            }
            catch (...)
            {
                result = "invalid argument";
            }
        }
        else
        {
            result = "unknown command";
        }
    }

    command = "";
    UpdateCommandLine(command, result);
    return false;
}

void Debugger::ShowHelpWindow()
{
    help_win->Clear();
    int r = 5;
    help_win->mvprint(2, 2, "Emulation mode");
    help_win->mvprint(3, 2, "--------------");
    help_win->mvprint(r++, 2, "r     | free run mode");
    help_win->mvprint(r++, 2, "h     | halt free run");
    help_win->mvprint(r++, 2, "space | step next instruction");

    help_win->mvprint(height - 4, 2, "Press x to return to Emulation Mode");
    help_win->Refresh();
}

void Debugger::UpdateVIAView()
{
    auto via = vectrex.GetVIA6522();
    auto registers = via.GetRegisterState();
    auto timer1 = via.GetTimer1();
    auto timer2 = via.GetTimer2();
    auto sr = via.GetShiftregister();

    via_win->Clear();

    via_win->mvprint(2, 2, "ORB:  %02x", registers.ORB);
    via_win->mvprint(3, 2, "ORA:  %02x", registers.ORA);
    via_win->mvprint(4, 2, "DDRB: %02x", registers.DDRB);
    via_win->mvprint(5, 2, "DDRA: %02x", registers.DDRA);
    via_win->mvprint(6, 2, "T1CL: %02x", registers.T1CL);
    via_win->mvprint(7, 2, "T1CH: %02x", registers.T1CH);
    via_win->mvprint(8, 2, "T1LL: %02x", registers.T1LL);
    via_win->mvprint(9, 2, "T1LH: %02x", registers.T1LH);
    via_win->mvprint(2, 15, "T2CL: %02x", registers.T2CL);
    via_win->mvprint(3, 15, "T2CH: %02x", registers.T2CH);
    via_win->mvprint(4, 15, "SR:   %02x", registers.SR);
    via_win->mvprint(5, 15, "ACR:  %02x", registers.ACR);
    via_win->mvprint(6, 15, "PCR:  %02x", registers.PCR);
    via_win->mvprint(7, 15, "IFR:  %02x", registers.IFR);
    via_win->mvprint(8, 15, "IER:  %02x", registers.IER);

    via_win->mvprint(10, 2, "CA: %d%d CB: %d%d", via.getCA1State(), via.getCA2State(), via.getCB1State(), via.getCB2State());
    via_win->mvprint(11, 2, "Timer1:  %05d (%s%s)", timer1.counter, timer1.enabled ? "on" : "off", timer1.one_shot ? ":1shot" : "");
    via_win->mvprint(12, 2, "Timer2:  %05d (%s%s)", timer2.counter, timer2.enabled ? "on" : "off", timer2.one_shot ? ":1shot" : "");
    via_win->mvprint(13, 2, "SR:         %02d (%s:%d)", sr.counter, sr.enabled ? "on" : "off", sr.shifted);

    via_win->Refresh();
}

void Debugger::UpdateVectorView()
{
    Vectorizer &vect = vectrex.GetVectorizer();
    auto &via = vectrex.GetVIA6522();
    auto via_registers = via.GetRegisterState();

    vect_win->mvprint(1, 2, "Beam %s",
                      vect.getBeamState().enabled ? "on" : "off");
    vect_win->mvprint(2, 2, "Pos:   %d, %d", vect.getBeamState().x - (33000/2), vect.getBeamState().y - (41000/2));
    vect_win->mvprint(3, 2, "Rate:  %d, %d", vect.getBeamState().rate_x, vect.getBeamState().rate_y);
    vect_win->mvprint(4, 2, "Count: %d", vect.countVectors());
    vect_win->mvprint(5, 2, "ZERO: %d, BLANK: %d, RAMP: %d", via.getCA2State(), via.getCB2State(), (via_registers.ORB >> 7));

    //
    if ((vect.getBeamState().rate_x != 0 || vect.getBeamState().rate_y != 0) && state == RUN && via.getCB2State())
        state = HALT;

    vect_win->Refresh();
}

std::string Debugger::str_format(const char *fmt, ...)
{
    va_list ap, ap2;
    va_start(ap, fmt);
    va_copy(ap2, ap);
    std::string out;

    int size = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (size > 0) {
        out = std::string(size+1, 0);
        vsnprintf(&out[0], out.size(), fmt, ap2);
        out.resize(static_cast<size_t>(size));
    }

    va_end(ap2);
    return out;
}
