#include "gfxutil.h"
#include <cmath>
#include <cstdlib>
#include <functional>
#include <inttypes.h>
#include "vectorizer.h"

void Vectorizer::Step(uint8_t porta, uint8_t portb, uint8_t zero_, uint8_t blank_)
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
                         [this, ramp_, zero_, new_integrator_x, new_integrator_y](uint64_t n){
                             UpdateSignals(ramp_, zero_, {new_integrator_x, new_integrator_y}, n);
                         });

#ifdef DEBUGGING
    min_x = std::min(axes.x, min_x);
    max_x = std::max(axes.x, max_x);
    min_y = std::min(axes.y, min_y);
    max_y = std::max(axes.y, max_y);
#endif

    cycles++;
}

void Vectorizer::UpdateSignals(uint8_t ramp_, uint8_t zero_, const integrators_t &integrators_, uint64_t remaining_nanos)
{
    float ramp_time_old = 0.0f;
    float ramp_time_new = 0.0f;

    if (!ramp_ && ramp) // ramp turning on
    {
        // the change is delayed, so the ramp time will only be what time is left from
        // the current cycle ie. the remainder time
        ramp_time_new = (float)(time_per_clock - (remaining_nanos / 1.0e9));
    }
    else if (ramp_ && !ramp) // ramp turning off
    {
        // when turning off, there is still a partial cycles amount of time to run for...
        ramp_time_old = (float)(remaining_nanos / 1.0e9);
    }
    else if (!ramp_ && !ramp) // still active
    {
        // if the integrators have changed, then you need one vector for the first part of the cycles
        // and another for the second part of the cycle
        ramp_time_old = (float) (remaining_nanos / 1.0e9);
        ramp_time_new = time_per_clock - ramp_time_old;
    }

    if (!zero_)
    {
        axes.zero();
    }

    // draw vectors using the OLD integrator value
    axes.integrate(ramp_time_old, integrators);

    vectors_.push_back({axes, blank, ramp, sample_z / 5.0f, cycles});

    // draw vectors using the NEW integrator values
    axes.integrate(ramp_time_new, integrators_);

    vectors_.push_back({axes, blank, ramp_, sample_z / 5.0f, cycles});

    zero = zero_;
    ramp = ramp_;
    integrators = integrators_;
}

//<editor-fold desc="Drawing Methods">

VectrexFramebuffer Vectorizer::getVectorBuffer()
{
    struct line_vector_t
    {
        float x0, y0, x1, y1;
        float intensity0, intensity1;
        uint64_t cycles0, cycles1;
        line_vector_t(axes_t pos, float intensity_, uint64_t cycles_)
        {
            x0 = x1 = pos.x;
            y0 = y1 = pos.y;
            intensity0 = intensity1 = intensity_;
            cycles0 = cycles1 = cycles_;
        };
        line_vector_t(axes_t pos, uint64_t cycles_)
        {
            x0 = x1 = pos.x;
            y0 = y1 = pos.y;
            intensity0 = intensity1 = 0.0f;
            cycles0 = cycles1 = cycles_;
        }
        void set_end(axes_t pos)
        {
            x1 = pos.x;
            y1 = pos.y;
        }
    };

    float x, y;
    // start with black
    framebuffer.fill();
#ifdef DEBUGGING
    debug_framebuffer.fill();
#endif

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
                line_vector_t debug_vect(vect->pos, vect->end_cycle);
                debug_to_draw.push_back(debug_vect);
#endif
                beam = false;
            }
            // Update line when the beam is on or if it's just turned off (that's the end of the line)
            line_vector_t &new_vect = to_draw.back();
            new_vect.set_end(vect->pos);
        }
        else
        {
            if (vect->blank) // beam has just turned on
            {
                line_vector_t new_vect(vect->pos, vect->intensity, vect->end_cycle);
                to_draw.push_back(new_vect);
                beam = true;
            }
#ifdef DEBUGGING
            if (!debug_to_draw.empty()) // is off, or just ending
            {
                // extend the debug vector
                line_vector_t &debug_vect = debug_to_draw.back();
                // beam may only be on for 1 cycle
                debug_vect.set_end(vect->pos);

                line_vector_t new_debug_vect(vect->pos, vect->intensity, vect->end_cycle);
                debug_to_draw.push_back(new_debug_vect);
            }
#endif
        }

        // fade the vector based on how long ago it was drawn
        vect->intensity -= ((cycles - vect->end_cycle) * (1.0f / decay_cycles));
        vect->end_cycle = cycles;
    }

    // remove all the vectors that have 0 intensity or less
    vectors_.erase(std::remove_if(vectors_.begin(), vectors_.end(),
                                  [](const Vector &v) { return v.intensity <= 0.0f; }), vectors_.end());

    for (const auto &vect: to_draw)
    {
        if (vect.intensity0 > 0.0f)
        {
            framebuffer.draw_line(vect.x0 * scale_factor, vect.y0 * scale_factor,
                                  vect.x1 * scale_factor, vect.y1 * scale_factor,
                                  vect.intensity0);
        }
    }

#ifdef DEBUGGING
    for (const auto &debug_vect: debug_to_draw)
    {
        debug_framebuffer.draw_line(debug_vect.x0 * scale_factor, debug_vect.y0 * scale_factor,
                                    debug_vect.x1 * scale_factor, debug_vect.y1 * scale_factor,
                                    color_t{1.0f, 0.0f, 0.0f, DEBUG_LINE_INTENSITY});
    }

    debug_framebuffer.draw_debug_grid({1.0f, 1.0f, 0.0f, 0.2f}, 0.5f / scale_factor, 1.0f / scale_factor);

    uint64_t dcycles = TimerUtil::nanos_to_cycles(signal_delay);
    uint64_t dremainder = signal_delay - TimerUtil::cycles_to_nanos(dcycles);

    debug_framebuffer.draw_debug_text(2, FRAME_HEIGHT-10, {0.0, 1.0, 0.0, 0.5f}, "delay: %" PRId64 "~ + %" PRId64 "ns decay: %d", dcycles, dremainder, decay_cycles);
    debug_framebuffer.draw_debug_text(2, FRAME_HEIGHT-20, {0.0, 1.0, 0.0, 0.5f}, "y: [%.2f, %.2f]", min_y, max_y);
    debug_framebuffer.draw_debug_text(2, FRAME_HEIGHT-30, {0.0, 1.0, 0.0, 0.5f}, "x: [%.2f, %.2f]", min_x, max_x);

    auto grid_size_len = snprintf(NULL, 0, "%.2f%%", scale_factor * 100);
    debug_framebuffer.draw_debug_text(FRAME_WIDTH - (grid_size_len * 9), 2, {0.0, 1.0, 0.0, 0.5f}, "%.2f%%", scale_factor * 100);

    debug_framebuffer.draw_debug_text(2, 2, {0.0, 1.0, 0.0, 0.5f}, "%'" PRId64, cycles);

    min_x =  10.0f;
    min_y =  10.0f;
    max_x = -10.0f;
    max_y = -10.0f;

    return debug_framebuffer + framebuffer;

#endif

    return framebuffer;
}

//</editor-fold>
