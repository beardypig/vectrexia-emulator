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
#ifndef VECTREXIA_VECTORDRAWING_H
#define VECTREXIA_VECTORDRAWING_H

static const int INTEGRATOR_UPDATE_DELAY = 12;

#include <stdint.h>
#include <vector>
#include <array>
#include <set>
#include <memory>
#include "updatetimer.h"

class Vectorizer
{
    const int VECTOR_WIDTH = 33000;
    const int VECTOR_HEIGHT = 41000;

    struct BeamState
    {
        int x, y;
        int rate_x, rate_y;
        bool enabled;

        // internal state
        unsigned int _x_axis, _y_axis, _offset;
        unsigned char _z_axis;
    } beam;

    struct Vector
    {
        int x0, y0, x1, y1;
        int rate_x, rate_y;
        float intensity;
        uint64_t start_cycle;
        uint64_t end_cycle;
    };

    Vector current_vector;
    std::array<float, 135300> framebuffer;
    uint64_t cycles = 0;
    UpdateTimer<int> rate_updates;
    UpdateTimer<uint8_t> signals;
    uint8_t ramp = 1;
    uint8_t zero = 1;
    uint8_t previous_ramp = 1;

    void integrate_axis();
    void center_beam();

    void draw_line(int x0, int y0, int x1, int y1, float starti, float endi);
    void start_vector();

    bool finish_vector();

public:
    std::vector<Vector> vectors_;
    Vectorizer();
    void BeamStep(unsigned char porta, unsigned char portb, unsigned char zero, unsigned char blank);
    const BeamState& getBeamState();

    size_t countVectors() { return vectors_.size(); }

    std::array<float, 135300> getVectorBuffer();

};


#endif //VECTREXIA_VECTORDRAWING_H
