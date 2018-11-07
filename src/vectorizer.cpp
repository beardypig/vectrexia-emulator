#include "gfxutil.h"
#include <cmath>
#include <cstdlib>
#include <functional>
#include <cinttypes>
#include "vectorizer.h"

constexpr float normalise(float value, float min, float max) {
  return (value - min) / (max - min);
}

void Vectorizer::Step(uint8_t porta, uint8_t portb, uint8_t zero, uint8_t blank)
{
  // porta is connected to the databus of the sound chip and DAC

  // PB0 - SWITCH
  // PB1 - SEL 0
  // PB2 - SEL 1
  // PB5 - COMPARE (input)
  // PB6 - CART Mapper
  // PB7 - RAMP

  dac.v = (float)porta * (10.0f/256.0f) - 5.0f;

  via.blank = blank == 0;
  via.zero = zero == 0;
  via.ramp = (portb >> 7u) == 0;

  via.sh = static_cast<bool>(portb & 0x1u);
  via.sel0 = static_cast<bool>((portb >> 1u) & 0x1u);
  via.sel1 = static_cast<bool>((portb >> 2u) & 0x1u);

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
