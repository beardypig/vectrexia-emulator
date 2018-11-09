#include "gfxutil.h"
#include <cmath>
#include <cstdlib>
#include <functional>
#include <cinttypes>
#include "vectorizer.h"

void Vectorizer::Step(uint8_t porta, uint8_t portb, uint8_t zero, uint8_t blank)
{
    // porta is connected to the databus of the sound chip and DAC

    // PB0 - SWITCH
    // PB1 - SEL 0
    // PB2 - SEL 1
    // PB5 - COMPARE (input)
    // PB6 - CART Mapper
    // PB7 - RAMP

    // X input
    dac.v = digital_to_analog(porta);

    uint8_t switch_ = (uint8_t)(portb & 0x1);
    uint8_t select = (uint8_t)((portb >> 1) & 0x3);

    via.blank = (blank == 0);
    via.zero  = (zero == 0);
    via.ramp  = (uint8_t)portb >> 7 == 0;

    if (!switch_)
    {
        switch (select)
        {
        case 0: // Y Axis Sample and Hold between [-5, +5]
            mpx.out1A = dac.v * 2;
            break;
        case 1: // Zero reference
            mpx.out1B = dac.v * 2;
            break;
        case 2: // Z Axis (brightness) Sample and Hold
            mpx.out1C = std::max(0.0f, -dac.v * 2);
            break;
        default:
            break;
        }
    }

  auto X0 = static_cast<float>(integrators.vX);
  auto Y0 = static_cast<float>(integrators.vY);

  integrators.step();

  auto intensity = normalise(static_cast<float>(integrators.vZ), -5.0f, +5.0f);
  auto X1 = static_cast<float>(integrators.vX);
  auto Y1 = static_cast<float>(integrators.vY);

  if (!via.blank) {
    vxgfx::draw_line<vxgfx::m_brightness>(vector_buffer, vp,
                                          X0, Y0, X1, Y1,
                                          vxgfx::pf_mono_t{intensity});
  }
}

VectorBuffer *Vectorizer::getVectorBuffer() {
  return &vector_buffer;
}
