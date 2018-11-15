/*
Copyright (C) 2016 beardypig, pelorat

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
#include "sw_vectorizer.h"

SWVectorizer::SWVectorizer(VIAPorts * via_, MPXPorts * mpx_, DACPorts * dac_)
    : Integrator(mpx_, via_, dac_)
{
    
}

void SWVectorizer::step()
{
    Integrator::step();
}

void SWVectorizer::draw()
{
    // TODO: interate and draw the vectors in the buffer
    // TODO: reset cycle count and state machine
}

void SWVectorizer::vec_Begin(uint32_t cycle, float x, float y, float z)
{
    current = {};
    current.count++;
    prevX = x;
    prevY = y;
    vertex.emplace_back(x, y, z);
}

void SWVectorizer::vec_End(uint32_t cycle, float x, float y, float z)
{
    vec_Vertex(cycle, x, y, z);
    current.length = sqrtf(current.length);
    vector.push_back(current);
}

void SWVectorizer::vec_Vertex(uint32_t cycle, float x, float y, float z)
{
    current.length += (x - prevX) * (x - prevX) + (y - prevY) * (y - prevY);
    current.count++;
    current.cycles = cycle;
    vertex.emplace_back(x, y, z);
    prevX = x;
    prevY = y;
}
