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
#include <ncurses.h>
#include <deque>
#include "vectrexia.h"

WINDOW *create_newwin(int height, int width, int starty, int startx);
void destroy_win(WINDOW *local_win);
void clear_win(WINDOW *local_win, const char * title);


static uint8_t read_mem(intptr_t ref, uint16_t addr)
{
    return reinterpret_cast<Vectrex*>(ref)->Read(addr);
}

static const int INSTRUCTION_LOG_SIZE = 10;

Vectrex vectrex;

int main(int argc, char *argv[])
{	WINDOW *program_win, *registers_win;
    int width, height;
    int ch;

    initscr();
    cbreak();
    curs_set(0);
    keypad(stdscr, TRUE);
    start_color();

    init_pair(1, COLOR_RED, COLOR_BLACK);

    getmaxyx(stdscr, height, width);

    // Reset the vectrex
    vectrex.Reset();
    auto disasm = M6809Disassemble();

    // Set memory callback for the disassembler
    disasm.SetReadCallback(read_mem, reinterpret_cast<intptr_t>(&vectrex));

    printw("Press F1 to exit");
    refresh();

    // Create windows for the various part
    program_win = create_newwin(height - 2, 30, 1, 1);
    registers_win = create_newwin(12, 20, 1, width-21);

    std::deque<std::string> instruction_log(INSTRUCTION_LOG_SIZE);

    while (1)
    {
        auto cpu = vectrex.GetM6809();
        auto registers = cpu.getRegisters();

        clear_win(registers_win, "Registers");
        mvwprintw(registers_win, 2, 2,  "A   : %02x", registers.A);
        mvwprintw(registers_win, 3, 2,  "B   : %02x", registers.A);
        mvwprintw(registers_win, 4, 2,  "D   : %04x", registers.D);
        mvwprintw(registers_win, 5, 2,  "X   : %04x", registers.X);
        mvwprintw(registers_win, 6, 2,  "Y   : %04x", registers.Y);
        mvwprintw(registers_win, 7, 2,  "SP  : %04x", registers.SP);
        mvwprintw(registers_win, 8, 2,  "USP : %04x", registers.USP);
        mvwprintw(registers_win, 9, 2,  "DP  : %02x", registers.DP);
        mvwprintw(registers_win, 10, 2, "PC  : %04x", registers.PC);
        wrefresh(registers_win);

        clear_win(program_win, "Program");
        uint16_t dis_addr = registers.PC;

        int log_i = 0;

        for (std::deque<std::string>::iterator it = instruction_log.begin(); it != instruction_log.end(); it++)
        {
            mvwprintw(program_win, 2 + log_i++, 2, " %s", (*it).c_str());
        }


        for (int i = 0; i < INSTRUCTION_LOG_SIZE; i++)
        {
            std::string disasm_instr = disasm.disasm(dis_addr);

            if (i == 0)
            {
                // This is the instruction that will run next
                if (instruction_log.size() == INSTRUCTION_LOG_SIZE)
                    instruction_log.pop_front();
                instruction_log.push_back(disasm_instr);
                wattron(program_win, COLOR_PAIR(1));
                mvwprintw(program_win, 2 + log_i + i, 2, ">%s", disasm_instr.c_str());
                wattroff(program_win, COLOR_PAIR(1));
            }
            else
            {
                // These instruction may run after
                mvwprintw(program_win, 2 + log_i + i, 2, " %s", disasm_instr.c_str());
            }
        }

        wrefresh(program_win);
        if ((ch = getch()) == KEY_F(1))
        {
            break;
        }
        // run for at least one cycle
        vectrex.Run(1);
    }
    endwin();			/* End curses mode		  */
    return 0;
}

WINDOW *create_newwin(int height, int width, int starty, int startx)
{	WINDOW *local_win;

    local_win = newwin(height, width, starty, startx);
    box(local_win, 0 , 0);		/* 0, 0 gives default characters
					 * for the vertical and horizontal
					 * lines			*/
    wrefresh(local_win);		/* Show that box 		*/

    return local_win;
}

void destroy_win(WINDOW *local_win)
{
    /* box(local_win, ' ', ' '); : This won't produce the desired
     * result of erasing the window. It will leave it's four corners
     * and so an ugly remnant of window.
     */
    wborder(local_win, ' ', ' ', ' ',' ',' ',' ',' ',' ');
    /* The parameters taken are
     * 1. win: the window on which to operate
     * 2. ls: character to be used for the left side of the window
     * 3. rs: character to be used for the right side of the window
     * 4. ts: character to be used for the top side of the window
     * 5. bs: character to be used for the bottom side of the window
     * 6. tl: character to be used for the top left corner of the window
     * 7. tr: character to be used for the top right corner of the window
     * 8. bl: character to be used for the bottom left corner of the window
     * 9. br: character to be used for the bottom right corner of the window
     */
    wrefresh(local_win);
    delwin(local_win);
}

void clear_win(WINDOW *local_win, const char * title)
{
    wclear(local_win);
    box(local_win, 0 , 0);
    mvwprintw(local_win, 0, 2, title);
}
