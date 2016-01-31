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
    ramp = (portb >> 7);

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
    beam.rate_x = (int)beam._x_axis - (int)beam._offset;
    beam.rate_y = (int)beam._offset - (int)beam._y_axis;

    if (!beam.enabled) {
        if (blank && beam_in_range()) {
            beam.enabled = true;
            // create a new vector
            current_vector = std::make_unique<Vector>(*this);
        }
    } else { // already drawing
        if (!blank && current_vector) { // turned the beam off, vector has finished
            beam.enabled = false;
            current_vector->end_cycle = cycles;
            vectors_.push_back(*current_vector);
            current_vector.release();
        } else if (current_vector->rate_x != beam.rate_x || current_vector->rate_y != beam.rate_y) {
            // vector parameters have changed, store the current vector and start a new one
            current_vector->end_cycle = cycles;
            vectors_.push_back(*current_vector);
            current_vector = std::make_unique<Vector>(*this);
        }
    }


    if (!ramp) // update the Axis if RAMP is active
        integrate_axis();
    if (!zero)
        center_beam();

    //printf("Beam state: x=%d, y=%d\n", beam.x, beam.y);

    // if the vector beam is still on, then continue drawing this vector
    if (blank && current_vector) {
        // update the end of the current vector
        current_vector->x1 = beam.x;
        current_vector->y1 = beam.y;
    }

    ++cycles;
}

bool Vectorizer::beam_in_range()
{
    return beam.x >= 0 && beam.x < VECTOR_WIDTH && \
           beam.y >= 0 && beam.y < VECTOR_HEIGHT;
}

void Vectorizer::integrate_axis()
{
    beam.x = std::min(std::max(beam.x + beam.rate_x, 0), VECTOR_WIDTH - 1); // limit to 0 - MAX_WIDTH
    beam.y = std::min(std::max(beam.y + beam.rate_y, 0), VECTOR_HEIGHT - 1);
}

void Vectorizer::center_beam()
{
    beam.x = VECTOR_WIDTH / 2;
    beam.y = VECTOR_HEIGHT / 2;
    beam.rate_x = 0;
    beam.rate_y = 0;

    beam._x_axis = 0x80;
    beam._y_axis = 0x80;
    beam._offset = 0x80;
}

// Create a new VectrexVector from the current BeamState
Vectorizer::Vector::Vector(Vectorizer & vbf)
{
    x0 = vbf.beam.x;
    y0 = vbf.beam.y;
    x1 = x0;
    y1 = y0;
    intensity = (float)(vbf.beam._z_axis)/128.0f;
    start_cycle = vbf.cycles;
    end_cycle = vbf.cycles;

    rate_x = vbf.beam.rate_x;
    rate_y = vbf.beam.rate_y;
}

const Vectorizer::BeamState &Vectorizer::getBeamState()
{
    return beam;
}

std::array<float, 135300> Vectorizer::getVectorBuffer()
{
    float x0, x1, y0, y1;
    framebuffer.fill(0.0f);

    //printf("Vector count = %d\n", (int)vectors_.size());

    for(auto &vect : vectors_)
    {
        x0 = (float)vect.x0 / (float) VECTOR_WIDTH * 330.0f;
        x1 = (float)vect.x1 / (float) VECTOR_WIDTH * 330.0f;
        y0 = (float)vect.y0 / (float) VECTOR_HEIGHT * 410.0f;
        y1 = (float)vect.y1 / (float) VECTOR_HEIGHT * 410.0f;

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
        auto pos = (y0 * 330) + x0;
        framebuffer[pos] = std::min(1.0f, framebuffer[pos] + intensity);

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
