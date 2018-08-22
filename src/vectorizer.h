#ifndef VECTREXIA_VECTORIZER2_H
#define VECTREXIA_VECTORIZER2_H

#include <cstdint>
#include <cstdarg>
#include <vector>
#include <array>
#include <string>
#include "gfxutil.h"
#include "updatetimer.h"


static const float VECTOR_MAX_V =  5.0f;
static const float VECTOR_MIN_V = -5.0f;
static const float time_per_clock = (float) (1.0f / 1.5e6);
static const float DEBUG_LINE_INTENSITY = 0.03f;
static const int FRAME_WIDTH  = 330;
static const int FRAME_HEIGHT = 410;

struct integrators_t
{
    float x = 0.0f,
            y = 0.0f;
};

struct axes_t
{
    float x = 0.0f,
            y = 0.0f;
    inline void zero() {
        x = 0.0f;
        y = 0.0f;
    }
    inline void integrate(float ramp_time, const integrators_t &integrators)
    {
        x += 10000 * ramp_time * integrators.x;
        y += 10000 * ramp_time * integrators.y;
    }
};

using VectorBuffer = vxgfx::framebuffer<FRAME_WIDTH, FRAME_HEIGHT, vxgfx::pf_mono_t>;
using DebugBuffer = vxgfx::framebuffer<FRAME_WIDTH, FRAME_HEIGHT, vxgfx::pf_argb_t>;

class Vectorizer
{
    struct Vector
    {
        axes_t pos;
        uint8_t blank, ramp;
        float intensity;
        uint64_t end_cycle;
    };

    // Sample and hold voltages (-5v - 5v) for Y axis and Z axis
    float sample_y = 0.0f;
    float sample_z = 0.0f;
    // not really a sample and hold, the value of X is whatever is whatever is out of the DAC ie. PORTA
    float sample_x = 0.0f;

    // DAC voltage for the X/Y axes
    axes_t axes;

    // voltages of the integrators
    integrators_t integrators;

    // The DAC is connected to PORTA, the MSB of the input is inverted
    // the output from the DAC will range from -2.5v to +2.5v
    inline float dac(uint8_t value)
    {
        return 2.5f - ((value ^ 0x80) * (5.0f / 256.0f));
    }

    template <typename T>
    auto clamp(T input, T min, T max)
    {
        return std::min(std::max(input, min), max);
    };

    // signals that control switches/beam
    uint8_t blank = 1;

    // signals that control switches/beam
    // RAMP and ZERO both control analog switches (IC305, MC54/74HC4066)
    // IC305 has VCC conencted to +5v and GND connected to -5v - according to the datasheet this will cause a delay
    // of ~400ns.
    // The transistor Q301 adds further delay of ~260ns.
    uint8_t zero = 1;
    uint8_t ramp = 1;

    // The DAC could add a delay of up to ~150ns.
    // Total delay:
    CallbackTimer signal_queue;

    uint64_t cycles = 0;

    vxgfx::viewport vp;

    std::vector<Vector> vectors_;
    VectorBuffer vector_buffer{};
    DebugBuffer debug_buffer{};

    float min_x, max_x, min_y, max_y;

public:
    void Step(uint8_t porta, uint8_t portb, uint8_t zero, uint8_t blank);

    // Returns a vxgfx::framebuffer<vxgfx::pf_mono_t>
    VectorBuffer *getVectorBuffer();

    // Returns a vxgfx::framebuffer<vxgfx::pf_argb_t>
    DebugBuffer *getDebugBuffer();

    uint64_t signal_delay = 7800;
    int decay_cycles = 40000; // a beam lasts for 40k cycles
    float scale_factor = 1.0f;
    float pan_offset_x = 0.0f;
    float pan_offset_y = 0.0f;

    void UpdateSignals(uint8_t ramp, uint8_t zero, const integrators_t &integrators, uint64_t remaining_nanos);
};


#endif //VECTREXIA_VECTORIZER2_H
