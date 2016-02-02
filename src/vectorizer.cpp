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
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include "vectorizer.h"

Vectorizer::Vectorizer()
{
    center_beam();
    beam.enabled = false;
    beam._z_axis = 0;
}

void Vectorizer::BeamStep(uint8_t porta, uint8_t portb, uint8_t zero, uint8_t blank)
{
    // porta is connected to the databus of the sound chip and DAC
    // portb 3+4 and ca1 for sound chip stuff

    // PB0 - SWITCH
    // PB1 - SEL 0
    // PB2 - SEL 1
    // PB5 - COMPARE (input)
    // PB6 - CART N/C? (input)
    // PB7 - RAMP
    uint8_t switch_, select, ramp;

    switch_ = (uint8_t)(portb & 0x1);
    select = (uint8_t)((portb >> 1) & 0x3);

    // update RAMP and ZERO in 12 cycles time
    signals.enqueue(cycles + INTEGRATOR_UPDATE_DELAY, &this->ramp, portb >> 7);
    signals.enqueue(cycles + INTEGRATOR_UPDATE_DELAY, &this->zero, zero);
    signals.enqueue(cycles + INTEGRATOR_UPDATE_DELAY, &this->previous_ramp, this->ramp);

    // The beams need to move to draw the vectors
    // PORTA is connected to the X Axis integrator, the DAC and the sound chip.
    // The X Axis is always set by PORTA, there is no enable
    beam._x_axis = (uint8_t)(porta ^ 0x80);

    // SWITCH (active low), update the rate of the vector beam
    if (switch_ == 0x00) {
        switch (select) {
            case 0: // Y Axis integrator
                beam._y_axis = (unsigned int)(porta ^ 0x80);
                break;
            case 1: // X,Y Axis integrator offset
                beam._offset = (unsigned int)(porta ^ 0x80);
                break;
            case 2: // Z Axis (brightness)
                beam._z_axis = (uint8_t)(porta > 128 ? 0 : porta);
                break;
            default:
                break;
        }
    }

    // beam rate (amount that is integrated each cycle)
    // the rates and the RAMP is delayed by ~12 cycles
    rate_updates.enqueue(cycles + INTEGRATOR_UPDATE_DELAY, &beam.rate_x, (int)beam._x_axis - (int)beam._offset);
    rate_updates.enqueue(cycles + INTEGRATOR_UPDATE_DELAY, &beam.rate_y, (int)beam._offset - (int)beam._y_axis);

    rate_updates.tick(cycles);
    signals.tick(cycles);

    if (!beam.enabled)
    {
        if (blank)
        {
            beam.enabled = true;
            // create a new vector
            start_vector();
        }
    }
    else
    {
        // already drawing
        if (!blank)
        {
            // turned the beam off, vector has finished
            beam.enabled = false;
            if (finish_vector())
                vectors_.push_back(current_vector);
        }
        else if (current_vector.rate_x != beam.rate_x || current_vector.rate_y != beam.rate_y)
        {
            // vector parameters have changed, store the current vector and start a new one
            if (finish_vector())
                vectors_.push_back(current_vector);
            start_vector();
        }
    }

    if (!this->ramp) // update the Axis if RAMP is active
    {
        if (previous_ramp != this->ramp)
        {
            if (!this->ramp) // just turned on
            {
                beam.x += 0.20f * beam.rate_x;
                beam.y += 0.20f * beam.rate_y;
            }
            else
            {
                beam.x -= 0.20f * beam.rate_x;
                beam.y -= 0.20f * beam.rate_y;
            }
        }
        integrate_axis();
        current_vector.x1 = beam.x;
        current_vector.y1 = beam.y;
        previous_ramp = this->ramp;
    }
    if (!this->zero)
        center_beam();

    ++cycles;
}

void Vectorizer::integrate_axis()
{
    beam.x += beam.rate_x;
    beam.y += beam.rate_y;
}

void Vectorizer::center_beam()
{
    beam.x = VECTOR_WIDTH / 2;
    beam.y = VECTOR_HEIGHT / 2;
}

const Vectorizer::BeamState &Vectorizer::getBeamState()
{
    return beam;
}

std::array<float, Vectorizer::FRAME_WIDTH * Vectorizer::FRAME_HEIGHT> Vectorizer::getVectorBuffer()
{
    float x0, x1, y0, y1;
    framebuffer.fill(0.0f);

    //printf("Vector count = %d\n", (int)vectors_.size());

    for(auto &vect : vectors_)
    {
        x0 = (float)vect.x0 / (float) VECTOR_WIDTH * FRAME_WIDTH;
        x1 = (float)vect.x1 / (float) VECTOR_WIDTH * FRAME_WIDTH;
        y0 = (float)vect.y0 / (float) VECTOR_HEIGHT * FRAME_HEIGHT;
        y1 = (float)vect.y1 / (float) VECTOR_HEIGHT * FRAME_HEIGHT;

        auto fade_cycles = cycles - vect.end_cycle;
        // a full intensity vector should fade in 40,000 cycles
        vect.intensity -= (fade_cycles*(1.0f/400000.0f));

        if (vect.intensity > 0.0f)
        {
            draw_line((int) x0, (int) y0, (int) x1, (int) y1, vect.intensity, vect.intensity);
        }
    }

    // remove all the vectors that have 0 intensity or less
    vectors_.erase(std::remove_if(vectors_.begin(), vectors_.end(),
                          [](Vector v) { return v.intensity <= 0.0f; }), vectors_.end());

    return framebuffer;
}

void Vectorizer::draw_line(int x0, int y0, int x1, int y1, float starti, float endi)
{
    int dx = abs(x1-x0);
    int dy = abs(y1-y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    int e2;
    float intensity = endi;

    while(1)
    {
        auto pos = (y0 * FRAME_WIDTH) + x0;
        if (y0 >= 0 && y0 < FRAME_HEIGHT && x0 >= 0 && x0 < FRAME_WIDTH)
        {
            framebuffer[pos] = std::min(1.0f, framebuffer[pos] + intensity);
        }
        if (x0 == x1 && y0 == y1)
            break;

        e2 = 2 * err;
        if (e2 > -dy)
        {
            err = err - dy;
            x0 = x0 + sx;
        }

        if (e2 < dx)
        {
            err = err + dx;
            y0 = y0 + sy;
        }

    }
}

void Vectorizer::start_vector()
{
    current_vector.x0 = beam.x;
    current_vector.y0 = beam.y;
    current_vector.x1 = current_vector.x0;
    current_vector.y1 = current_vector.y0;

    current_vector.intensity = (float)(beam._z_axis)/128.0f;
    current_vector.start_cycle = cycles;
    current_vector.end_cycle = cycles;

    current_vector.rate_x = beam.rate_x;
    current_vector.rate_y = beam.rate_y;
}

bool Vectorizer::finish_vector()
{
    current_vector.end_cycle = cycles;
    current_vector.x1 = beam.x;
    current_vector.y1 = beam.y;

    //return vector_cycles > 0;
}
