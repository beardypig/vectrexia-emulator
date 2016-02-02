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
#ifndef VECTREXIA_UPDATETIMER_H
#define VECTREXIA_UPDATETIMER_H

#include <stdint.h>
#include <deque>

template<typename T>
class UpdateTimer
{
    struct data
    {
        uint64_t cycles;
        T *ptr;
        T value;
    };

    std::deque<data> queue;
public:
    // enqueue and item to be updated at a later time
    void enqueue(uint64_t cycles, T *ptr, T value)
    {
        if (queue.size())
        {
            // insert the item in to the queue so that the queue is sorted by cycles, lowest first
            for (auto it = queue.end(); it != queue.begin(); --it)
            {
                if (cycles < it->cycles)
                {
                    queue.insert(it, {cycles, ptr, value});
                    break;
                }
            }
        }
        else
        {
            queue.push_back({cycles, ptr, value});
        }
    }

    void tick(uint64_t cycles)
    {
        while (queue.size() && queue.front().cycles <= cycles)
        {
            auto &item = queue.front();
            *(item.ptr) = item.value;
            queue.pop_front();
        }
    }
};


#endif //VECTREXIA_UPDATETIMER_H
