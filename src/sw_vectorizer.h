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

#ifndef SWVECTORIZER_H
#define SWVECTORIZER_H

#include "integrator.h"
#include "gfxutil.h"

static const int FRAME_WIDTH = 330;
static const int FRAME_HEIGHT = 410;
using framebuffer_t = vxgfx::framebuffer<FRAME_WIDTH, FRAME_HEIGHT, vxgfx::pf_mono_t>;

class SWVectorizer : public Integrator<SWVectorizer>
{
public:
    
    // Constructor
    SWVectorizer(VIAPorts * via_, MPXPorts * mpx_, DACPorts * dac_);
    
    // Stepping function
    void step();

    // Draw frame function
    void draw();

protected:
    
    // Invoked when BLANK turns OFF
    void vec_Begin(uint32_t cycle, float x, float y, float z);

    // Invoked when BLANK turns ON
    void vec_End(uint32_t cycle, float x, float y, float z);

    // Invoked when beam state changes (direction, brightness)
    void vec_Vertex(uint32_t cycle, float x, float y, float z);

    // Framebuffer
    framebuffer_t frameBuffer;
};

#endif // !SWVECTORIZER_H
