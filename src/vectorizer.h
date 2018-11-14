#ifndef VECTREXIA_VECTORIZER2_H
#define VECTREXIA_VECTORIZER2_H

#include <cstdint>
#include <cstdarg>
#include <vector>
#include <array>
#include <string>
#include "gfxutil.h"
#include "integrator.h"

constexpr float time_per_clock = (float) (1.0f / 1.5e6);
static const float DEBUG_LINE_INTENSITY = 0.03f;
static const int FRAME_WIDTH  = 330;
static const int FRAME_HEIGHT = 410;

constexpr float normalise(float value, float min, float max) {
    return (value - min) / (max - min);
}

constexpr float digital_to_analog(uint8_t value) {
    return 2.5f - ((value ^ 0x80) * (5.0f / 256.0f));
}

using VectorBuffer = vxgfx::framebuffer<FRAME_WIDTH, FRAME_HEIGHT, vxgfx::pf_mono_t>;

class Vectorizer
{
private:
  vxgfx::viewport vp;
  VectorBuffer vector_buffer{};
  MPXPorts mpx{};
  VIAPorts via{};
  DACPorts dac{};
  XYZAxisIntegrators integrators = XYZAxisIntegrators(&mpx, &via, &dac);

public:
  void Step(uint8_t porta, uint8_t portb, uint8_t zero, uint8_t blank);

  // Returns a vxgfx::framebuffer<vxgfx::pf_mono_t>
  VectorBuffer *getVectorBuffer();
};


#endif //VECTREXIA_VECTORIZER2_H
