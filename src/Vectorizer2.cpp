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

    signal_queue.tick(cycles);

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
                          zero_,
                          {new_integrator_x,
                           new_integrator_y}
                         });


#ifdef DEBUGGING
    min_x = std::min(axes.x, min_x);
    max_x = std::max(axes.x, max_x);
    min_y = std::min(axes.y, min_y);
    max_y = std::max(axes.y, max_y);
#endif

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
        ramp_time_new = (float)(time_per_clock - (remaining_nanos / 1.0e9));
    }
    else if (data.ramp && !ramp) // ramp turning off
    {
        // when turning off, there is still a partial cycles amount of time to run for...
        ramp_time_old = (float) (time_per_clock + (remaining_nanos / 1.0e9));//(float)(remaining_nanos / 1.0e9);
    }
    else if (!data.ramp && !ramp) // still active
    {
        // if the integrators have changed, then you need one vector for the first part of the cycles
        // and another for the second part of the cycle
        ramp_time_old = (float) (remaining_nanos / 1.0e9);
        ramp_time_new = time_per_clock - ramp_time_old;
    }

    if (!data.zero)
    {
        axes.zero();
    }

    // draw vectors using the OLD integrator value
    axes.integrate(ramp_time_old, integrators);

#ifndef DEBUGGING
    if (blank_)
#endif
    vectors_.push_back({axes, blank, ramp, sample_z / 5.0f, cycles});

    // draw vectors using the NEW integrator values
    axes.integrate(ramp_time_new, data.integrators);

#ifndef DEBUGGING
    if (blank_)
#endif
    vectors_.push_back({axes, blank, data.ramp, sample_z / 5.0f, cycles});

    zero = data.zero;
    ramp = data.ramp;
    integrators = data.integrators;
}

//<editor-fold desc="Drawing Methods">

Vectorizer2::framebuffer_t Vectorizer2::getVectorBuffer()
{
    struct line_vector_t
    {
        float x0, y0, x1, y1;
        float intensity0, intensity1;
        uint64_t cycles0, cycles1;
    };

    float x, y;
    // start with black
    framebuffer.fill({0.0f, 0.0f, 0.0f, 0.0f});

    std::vector<line_vector_t> to_draw;
    std::vector<line_vector_t> debug_to_draw;
    bool beam = false;

    for (auto vect = vectors_.begin(); vect != vectors_.end(); vect++)
    {
        if (beam)
        {
            if (!vect->blank)
            {
#ifdef DEBUGGING
                line_vector_t debug_vect;
                // beam may only be on for 1 cycle
                debug_vect.x0 = vect->pos.x;
                debug_vect.y0 = vect->pos.y;
                debug_vect.x1 = vect->pos.x;
                debug_vect.y1 = vect->pos.y;
                debug_vect.intensity0 = 0.0f;
                debug_vect.cycles0 = 0;
                debug_to_draw.push_back(debug_vect);
#endif
                beam = false;
            }
            // Update line when the beam is on or if it's just turned off (that's the end of the line)
            auto &new_vect = to_draw.back();
            new_vect.x1 = vect->pos.x;
            new_vect.y1 = vect->pos.y;
            new_vect.intensity1 = vect->intensity;
            new_vect.cycles1 = vect->end_cycle;
        }
        else
        {
            if (vect->blank) // beam has just turned on
            {
                line_vector_t new_vect;
                // beam may only be on for 1 cycle
                new_vect.x1 = new_vect.x0 = vect->pos.x;
                new_vect.y1 = new_vect.y0 = vect->pos.y;
                new_vect.intensity0 = vect->intensity;
                new_vect.cycles0 = vect->end_cycle;
                to_draw.push_back(new_vect);
                beam = true;
            }
#ifdef DEBUGGING
            if (!debug_to_draw.empty()) // is off, or just ending
            {
                // extend the debug vector
                auto &debug_vect = debug_to_draw.back();
                // beam may only be on for 1 cycle
                debug_vect.x1 = vect->pos.x;
                debug_vect.y1 = vect->pos.y;
                debug_vect.intensity1 = 0.0f;
                debug_vect.cycles1 = 0;

                line_vector_t new_debug_vect;
                // beam may only be on for 1 cycle
                new_debug_vect.x0 = new_debug_vect.x1 = vect->pos.x;
                new_debug_vect.y0 = new_debug_vect.y1 = vect->pos.y;
                new_debug_vect.intensity0 = 0.0f;
                new_debug_vect.cycles0 = 0;
                debug_to_draw.push_back(new_debug_vect);
            }
        }
#endif

        vect->intensity -= ((cycles - vect->end_cycle) * (1.0f / decay_cycles)); // life time
        vect->end_cycle = cycles;
    }


    for (auto &vect: to_draw)
    {
        if (vect.intensity0 > 0.0f)
        {
            draw_line(vect.x0, vect.y0, vect.x1, vect.y1, vect.intensity0);
        }
    }

#ifdef DEBUGGING
    for (auto &debug_vect: debug_to_draw)
    {
        draw_debug_line(debug_vect.x0, debug_vect.y0, debug_vect.x1, debug_vect.y1, DEBUG_LINE_INTENSITY);
    }
#endif


    // remove all the vectors that have 0 intensity or less
    vectors_.erase(std::remove_if(vectors_.begin(), vectors_.end(),
                                  [](const Vector &v) { return v.intensity <= 0.0f; }), vectors_.end());

#ifdef DEBUGGING
    draw_debug_grid({1.0f, 1.0f, 0.0f, 0.2f}, 0.5f / scale_factor, 1.0f / scale_factor);

    draw_debug_text(2, FRAME_HEIGHT-10, {0.0, 1.0, 0.0, 0.5f}, "delay: %" PRId64 "ns decay: %d", signal_delay, decay_cycles);
    draw_debug_text(2, FRAME_HEIGHT-20, {0.0, 1.0, 0.0, 0.5f}, "y: [%.2f, %.2f]", min_y, max_y);
    draw_debug_text(2, FRAME_HEIGHT-30, {0.0, 1.0, 0.0, 0.5f}, "x: [%.2f, %.2f]", min_x, max_x);

    auto grid_size_len = snprintf(NULL, 0, "%.2f%%", scale_factor * 100);
    draw_debug_text(FRAME_WIDTH - (grid_size_len * 9), 2, {0.0, 1.0, 0.0, 0.5f}, "%.2f%%", scale_factor * 100);

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

void Vectorizer2::draw_line(framebuffer_t &fb, float fx0, float fy0, float fx1, float fy1, color_t color)
{
    int x0 = (int) (normalise((fx0 + pan_offset_x) * scale_factor, VECTOR_MIN_V / 2, VECTOR_MAX_V / 2) * (float) FRAME_WIDTH);
    int y0 = (int) (normalise((fy0 + pan_offset_y) * scale_factor, VECTOR_MIN_V, VECTOR_MAX_V) * (float) FRAME_HEIGHT);
    int x1 = (int) (normalise((fx1 + pan_offset_x) * scale_factor, VECTOR_MIN_V / 2, VECTOR_MAX_V / 2) * (float) FRAME_WIDTH);
    int y1 = (int) (normalise((fy1 + pan_offset_y) * scale_factor, VECTOR_MIN_V, VECTOR_MAX_V) * (float) FRAME_HEIGHT);
    draw_line(fb, x0, y0, x1, y1, color);
}

void Vectorizer2::draw_line(float x0, float y0, float x1, float y1, float endi)
{
    draw_line(framebuffer, x0, y0, x1, y1, endi);
}

void Vectorizer2::draw_line(framebuffer_t &fb, float x0, float y0, float x1, float y1, float endi)
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
        if (x0 < FRAME_WIDTH && x0 > 0 && y0 < FRAME_HEIGHT && y0 > 0)
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
void Vectorizer2::draw_debug_line(float x0, float y0, float x1, float y1, float endi)
{
    draw_line(debug_framebuffer, x0, y0, x1, y1, color_t{1.0f, 0.0f, 0.0f, endi});
}

void Vectorizer2::draw_debug_line(int x0, int y0, int x1, int y1, float endi)
{
    draw_line(debug_framebuffer, x0, y0, x1, y1, color_t{1.0f, 0.0f, 0.0f, endi});
}

void Vectorizer2::draw_debug_text(int x, int y, color_t color, const char* fmt, ...)
{
    char buf[2000];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, 2000, fmt, args);
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

// draw grid at voltage inteval
void Vectorizer2::draw_debug_grid(color_t line, float xi, float yi)
{
    // left + right
    draw_line(debug_framebuffer, VECTOR_MIN_V / 2, VECTOR_MIN_V, VECTOR_MIN_V / 2, VECTOR_MAX_V, line);
    draw_line(debug_framebuffer, VECTOR_MAX_V / 2, VECTOR_MIN_V, VECTOR_MAX_V / 2, VECTOR_MAX_V, line);

    for (float x = (VECTOR_MIN_V / 2) + xi; x < VECTOR_MAX_V / 2.0f; x += xi)
    {
        draw_line(debug_framebuffer, x, VECTOR_MIN_V, x, VECTOR_MAX_V, line);
    }

    // top + bottom
    draw_line(debug_framebuffer, VECTOR_MIN_V / 2, VECTOR_MIN_V, VECTOR_MAX_V / 2, VECTOR_MIN_V, line);
    draw_line(debug_framebuffer, VECTOR_MIN_V / 2, VECTOR_MAX_V, VECTOR_MAX_V / 2, VECTOR_MAX_V, line);

    for (float y = VECTOR_MIN_V + yi; y < VECTOR_MAX_V; y += yi)
    {
        draw_line(debug_framebuffer, VECTOR_MIN_V / 2.0f, y, VECTOR_MAX_V / 2.0f, y, line);
    }
}
#endif

//</editor-fold>

