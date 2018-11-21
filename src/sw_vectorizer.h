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

#ifndef SW_VECTORIZER_H
#define SW_VECTORIZER_H

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
    void draw(uint64_t cycles_per_frame);

    framebuffer_t *getFrameBuffer() {
        return &frameBuffer;
    }

protected:

    friend class Integrator<SWVectorizer>;

    struct vertex_t {
        vertex_t(float x_, float y_, float z_)
            : x(x_), y(y_), z(z_) { }
        float x;
        float y;
        float z;
    };

    struct vector_t {
        uint32_t count = 0;
        uint32_t cycles = 0;
        float length = 0.0f;
    };

    // Invoked when BLANK turns OFF
    void vec_Begin(uint32_t cycle, float x, float y, float z);

    // Invoked when BLANK turns ON
    void vec_End(uint32_t cycle, float x, float y, float z);

    // Invoked when beam state changes (direction, brightness)
    void vec_Vertex(uint32_t cycle, float x, float y, float z);

    framebuffer_t frameBuffer;
    std::vector<vertex_t> vertex;   // list of vertexes
    std::vector<vector_t> vector;   // list of vectors
    vector_t current{};             // the current vector being processed
    float prevX = 0.0f;
    float prevY = 0.0f;
    uint32_t startCycle = 0;
};

#endif // !SW_VECTORIZER_H
