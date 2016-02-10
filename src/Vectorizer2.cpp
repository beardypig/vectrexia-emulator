#include <cmath>
#include <cstdlib>
#include <functional>
#include <inttypes.h>
#include <stdarg.h>
#include "Vectorizer2.h"

void Vectorizer2::Step(uint8_t porta, uint8_t portb, uint8_t zero_, uint8_t blank_)
{
    // porta is connected to the databus of the sound chip and DAC

    // PB0 - SWITCH
    // PB1 - SEL 0
    // PB2 - SEL 1
    // PB5 - COMPARE (input)
    // PB6 - CART N/C? (input)
    // PB7 - RAMP

    uint8_t switch_ = (uint8_t)(portb & 0x1);
    uint8_t select = (uint8_t)((portb >> 1) & 0x3);
    blank = blank_;
    zero = zero_;

    // sample x is always set
    float sample_v = dac(porta);
    float ref_0 = 0.0f;

    // DAC sample is between -2.5, +2.5
    sample_x = sample_v;

    if (!switch_)
    {
        switch (select)
        {
            case 0: // Y Axis Sample and Hold between [-5, +5]
                sample_y = sample_v * 2;
                break;
            case 1: // Zero reference
                ref_0 = sample_v * 2;
                break;
            case 2: // Z Axis (brightness) Sample and Hold
                sample_z = std::max(0.0f, -sample_v * 2); // clamp to [0, 5]
                break;
            default:
                break;
        }
    }

    auto new_integrator_x = ref_0 - sample_x;
    auto new_integrator_y = sample_y - ref_0;

    uint8_t ramp_ = (uint8_t)portb >> 7;
    // update RAMP and integrators in 7800ns
    signal_queue.enqueue(cycles,
                         signal_delay,
                         [this](signal_data_t d, uint64_t n){
                             UpdateSignals(d, n);
                         },
                         {ramp_,
                          {new_integrator_x,
                           new_integrator_y}
                         });

    signal_queue.tick(cycles);

#ifdef DEBUGGING
    min_x = std::min(axes.x, min_x);
    max_x = std::max(axes.x, max_x);
    min_y = std::min(axes.y, min_y);
    max_y = std::max(axes.y, max_y);
#endif

    if (!zero_)
    {
        axes.zero();
    }

    cycles++;
}

void Vectorizer2::UpdateSignals(signal_data_t data, uint64_t remaining_nanos)
{
    float ramp_time_old = 0.0f;
    float ramp_time_new = 0.0f;

    if (!data.ramp && ramp) // ramp turning on
    {
        // the change is delayed, so the ramp time will only be what time is left from
        // the current cycle ie. the remainder time
        ramp_time_new = (float)(remaining_nanos / 1.0e9);
    }
    else if (data.ramp && !ramp) // ramp turning off
    {
        // when turning off, there is still a partial cycles amount of time to run for...
        ramp_time_old = (float)(time_per_clock - (remaining_nanos / 1.0e9));
    }
    else if (!data.ramp && !ramp) // still active
    {
        // if the integrators have changed, then you need one vector for the first part of the cycles
        // and another for the second part of the cycle
        ramp_time_old = (float)(time_per_clock - (remaining_nanos / 1.0e9));
        ramp_time_new = (float)(remaining_nanos / 1.0e9);
    }

    // draw vectors using the OLD integrator value
#ifndef DEBUGGING
    if (blank_)
#endif
        vectors_.push_back({ramp_time_old, axes, integrators, blank, sample_z / 5.0f, cycles});
    axes.integrate(ramp_time_old, integrators);

    // draw vectors using the NEW integrator values
#ifndef DEBUGGING
    if (blank_)
#endif
        vectors_.push_back({ramp_time_new, axes, data.integrators, blank, sample_z / 5.0f, cycles});
    axes.integrate(ramp_time_new, data.integrators);

    ramp = data.ramp;
    integrators = data.integrators;
}

//<editor-fold desc="Drawing Methods">

Vectorizer2::framebuffer_t Vectorizer2::getVectorBuffer()
{
    float x0, x1, y0, y1, nx, ny;
    framebuffer.fill({0.0f, 0.0f, 0.0f, 0.0f});
    //printf("Drawing %d vectors...\n", vectors_.size());

    for(auto &vect : vectors_)
    {
        // X is between -2.5 and 2.5
        // Y is between -5 and 5
        x0 = normalise(vect.axes.x * scale_factor, VECTOR_MIN_V / 2, VECTOR_MAX_V / 2) * (float)FRAME_WIDTH;
        y0 = normalise(vect.axes.y * scale_factor, VECTOR_MIN_V, VECTOR_MAX_V) * (float)FRAME_HEIGHT;

        // calculate the new x/y axis values
        axes_t new_axes = vect.axes;
        new_axes.integrate(vect.ramp_time, vect.integrators);

        x1 = normalise(new_axes.x * scale_factor, VECTOR_MIN_V / 2, VECTOR_MAX_V / 2) * (float)FRAME_WIDTH;
        y1 = normalise(new_axes.y * scale_factor, VECTOR_MIN_V, VECTOR_MAX_V) * (float)FRAME_HEIGHT;

        vect.intensity -= ((cycles - vect.end_cycle) * (1.0f / decay_cycles)); // life time
        vect.end_cycle = cycles;

        if (x0 < FRAME_WIDTH && y0 < FRAME_HEIGHT && x1 < FRAME_WIDTH && y1 < FRAME_HEIGHT && x0 >= 0 && y0 >= 0 && x1 >= 0 && y1 >= 0)
        {
            if (vect.blank)
            {
                if (vect.intensity > 0.0f)
                {
                    draw_line((int) x0, (int) y0, (int) x1, (int) y1, vect.intensity);
                }
            }
#ifdef DEBUGGING
            else
            {
                draw_debug_line((int) x0, (int) y0, (int) x1, (int) y1, DEBUG_LINE_INTENSITY);
            }
#endif
        }
    }
    // remove all the vectors that have 0 intensity or less
    vectors_.erase(std::remove_if(vectors_.begin(), vectors_.end(),
                                  [](const Vector &v) { return v.intensity <= 0.0f; }), vectors_.end());

#ifdef DEBUGGING
    draw_debug_grid({1.0f, 1.0f, 0.0f, 0.2f}, 10, 10);
    draw_debug_text(2, FRAME_HEIGHT-10, {0.0, 1.0, 0.0, 0.5f}, "delay: %" PRId64 "ns decay: %d", signal_delay, decay_cycles);
    draw_debug_text(2, FRAME_HEIGHT-20, {0.0, 1.0, 0.0, 0.5f}, "y: [%.2f, %.2f]", min_y, max_y);
    draw_debug_text(2, FRAME_HEIGHT-30, {0.0, 1.0, 0.0, 0.5f}, "x: [%.2f, %.2f]", min_x, max_x);

    draw_debug_text(2, 2, {0.0, 1.0, 0.0, 0.5f}, "%'" PRId64, cycles);

    min_x =  10.0f;
    min_y =  10.0f;
    max_x = -10.0f;
    max_y = -10.0f;

    framebuffer_t fb = debug_framebuffer;
    debug_framebuffer.fill({0});

    // combine a cpoy of the framebuffer and the debug framebuffer...
    for (int p = 0; p < FRAME_HEIGHT * FRAME_WIDTH; p++)
    {
        fb[p] += framebuffer[p];
    }

    return fb;

#endif

    return framebuffer;
}

void Vectorizer2::draw_line(int x0, int y0, int x1, int y1, float endi)
{
    draw_line(framebuffer, x0, y0, x1, y1, endi);
}

void Vectorizer2::draw_line(framebuffer_t &fb, int x0, int y0, int x1, int y1, float endi)
{
    draw_line(fb, x0, y0, x1, y1, color_t{1.0f, 1.0f, 1.0f, endi});
}

void Vectorizer2::draw_line(framebuffer_t &fb, int x0, int y0, int x1, int y1, color_t color)
{
    int dx = abs(x1-x0);
    int dy = abs(y1-y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    int e2;

    while(1)
    {
        auto pos = (y0 * FRAME_WIDTH) + x0;
        fb[pos] += color;
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

#ifdef DEBUGGING
// Debug only methods
void Vectorizer2::draw_debug_line(int x0, int y0, int x1, int y1, float endi)
{
    draw_line(debug_framebuffer, x0, y0, x1, y1, color_t{1.0f, 0.0f, 0.0f, endi});
}

void Vectorizer2::draw_debug_text(int x, int y, color_t color, const char* fmt, ...)
{
    char buf[2000];
    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    draw_debug_text(x, y, color, std::string(buf));
    va_end(args);
}


void Vectorizer2::draw_debug_text(int x, int y, color_t color, std::string message)
{
    for (const char &c: message)
    {
        for (int fy=0; fy < 8; fy++) {
            for (int fx=0; fx < 8; fx++) {
                auto fchar = font8x8_basic[c & 0x7f][fy];
                if (fchar & (1 << fx))
                    debug_framebuffer[((y + fy) * FRAME_WIDTH) + x + fx] += color;
            }
        }
        x += 8 + 1; // + 1 for spacing
    }
}

void Vectorizer2::draw_debug_grid(color_t line, int xn=4, int yn=4)
{
    for (int x = FRAME_WIDTH/xn; x < FRAME_WIDTH; x += FRAME_WIDTH/xn)
       draw_line(debug_framebuffer, x, 0, x, FRAME_HEIGHT-1, line);

    for (int y = FRAME_WIDTH/xn; y < FRAME_HEIGHT; y += FRAME_WIDTH/xn)
       draw_line(debug_framebuffer, 0, y, FRAME_WIDTH-1, y, line);
}
#endif

//</editor-fold>

