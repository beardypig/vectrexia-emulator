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
#ifndef VECTREXIA_DEBUGGER_H
#define VECTREXIA_DEBUGGER_H


#include <memory>
#include <deque>
#include <set>
#include <vectrexia.h>
#include "window.h"

enum debug_mode_t
{
    HALT,
    RUN,
    STEP,
    MEGA_STEP
};

enum program_mode_t
{
    NORMAL,
    HELP
};


class Debugger
{
    const int INSTRUCTION_LOG_SIZE = 10;

    std::unique_ptr<Window> register_win;
    std::unique_ptr<Window> program_win;
    std::unique_ptr<Window> stack_win;
    std::unique_ptr<Window> user_stack_win;
    std::unique_ptr<Window> command_win;
    std::unique_ptr<Window> help_win;
    std::unique_ptr<Window> via_win;

    int width, height;
    long clk;
    uint16_t prev_pc;

    // machine state
    Vectrex vectrex;
    M6809Disassemble disasm;

    debug_mode_t state = HALT; // start in halted mode
    std::deque<std::string> instruction_log;
    std::vector<std::string> command_history;
    int command_history_i = 0;
    std::set<uint16_t> breakpoints;
    std::set<uint16_t> temp_breakpoints;
    std::string command;
    std::string commandt;
    bool capture = false;
    bool ended = false;
    program_mode_t mode = NORMAL;

    inline int parse_addr(std::string addr)
    {
        // starts with $ or 0x
        if (addr.substr(0, 1).compare("$") == 0)
            return std::stoi(addr.substr(1), nullptr, 16);
        else if (addr.substr(0, 2).compare("0x") == 0)
            return std::stoi(addr.substr(2), nullptr, 16);
    }

public:
    Debugger();
    ~Debugger();
    void Reset();
    void Refresh();
    void Run();

    void UpdatedRegisters();
    void UpdateProgramView();
    void UpdateStackViews();
    void UpdateVIAView();

    void UpdateStackView(uint16_t sp, Window *window);

    void SetMode(debug_mode_t mode);

    void UpdateCommandLine(std::string command, std::string result = "");
    void CaptureKey(int ch);
    bool RunCommand();
    void ShowHelpWindow();

    bool is_ended();

    std::string str_format(const char *fmt, ...);
};


#endif //VECTREXIA_DEBUGGER_H
