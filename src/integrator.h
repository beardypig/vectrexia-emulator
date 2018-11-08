#include <cassert>
#include <vector>
#include <tuple>
#include <variant>
#include <optional>
#include <fstream>

template<class T, class Compare>
constexpr const T& clamp(const T& v, const T& lo, const T& hi, Compare comp) {
  return assert(!comp(hi, lo)),
      comp(v, lo) ? lo : comp(hi, v) ? hi : v;
}

template<class T>
constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
  return clamp(v, lo, hi, std::less<>());
}

struct MPXPorts
{
  float out1A = 0.0f;    // Y-axis voltage
  float out1B = 0.0f;    // Integrator offset
  float out1C = 0.0f;    // Z-axis voltage
  float out1D = 0.0f;    // Sound
  float out2 = 0.0f;     // Compare

  void reset() {
    out1A = 0.0f;
    out1B = 0.0f;
    out1C = 0.0f;
    out1D = 0.0f;
    out2 = 0.0f;
  }
};

struct DACPorts
{
  float v = 0.0f;        // DAC voltage
};

struct VIAPorts
{
  bool sh = false;
  bool sel0 = false;
  bool sel1 = false;
  bool zero = false;
  bool ramp = false;
  bool compare = false;
  bool blank = false;
};


class XYZAxisIntegrators {
private:

  // Constants
  const float r_CD4052B = 125.0f;         // Multiplexer impedance
  const float r_LF347N = 200.0f;          // Op-amp impedance
  const float r_MC1408P8 = 1000.0f;       // DAC impedance
  const float r_CD4066B = 125.0f;         // Switch impedance
  const double c_C304 = 1 / 100000000.0;  // Y buffer capacitance: 0.01uF
  const double c_C306 = 1 / 100000000.0;  // Z buffer capacitance: 0.01uF
  const double c_C312 = 1 / 100000000.0;  // Y integrator capacitance: 0.01uF
  const double c_C313 = 1 / 100000000.0;  // X integrator capacitance: 0.01uF
  const float r_RAMP = 15000.0f;          // RAMP delay resistance (R323)*
  const float r_ZERO = 15000.0f;          // ZERO delay resistance (R325)*
  const double c_RAMP = 1 / 100000000.0;  // RAMP delay capacitance*
  const double c_ZERO = 1 / 100000000.0;  // ZERO delay capacitance*
  const float r_R316_319 = 10000.0f;      // RAMP resistor value
  const float r_R332_334 = 3300000.f;     // Zero-ref resistor value
  const float r_R317 = 220.0f;            // Y integrator discharge capacitor
  const float r_R320 = 220.0f;            // X integrator discharge capacitor

  // * = We'll simulate the CD4066B delay by pretending it's a capacitor that needs to
  // charge over a specific threshold percentage. To adjust the delay we'll adjust the
  // threshold. The threshold is a percentage of 5v.

  // Connections
  MPXPorts * mpx = nullptr;
  VIAPorts * via = nullptr;
  DACPorts * dac = nullptr;

  // Settings
  float Vdd = +5.0f;          // Vs+
  float Vss = -5.0f;          // Vs-
  float dt = 1 / 1500000.0f;  // Clock; 1.5Mhz
  float v_r333 = 0.0f;        // 0-ref voltage Y
  float v_r335 = 0.0f;        // 0-ref voltage X
  float d_ZERO = 0.63f;       // ZERO delay threshold*
  float d_RAMP = 0.63f;       // RAMP delay threshold*
  float d_C304 = 0.000000001f;  // C304 decay (Y). Real value unknown...
  float d_C306 = 0.000000001f;  // C306 decay (Z). Real value unknown...
  float d_C312 = 0.000000001f;  // C312 decay (Y). Real value unknown...
  float d_C313 = 0.000000001f;  // C313 decay (X). Real value unknown...

public:

  // State variables
  double v_C312 = 0.0f;        // Y integrator capacitor voltage.
  double v_C313 = 0.0f;        // X integrator capacitor voltage.
  double v_C304 = 0.0f;        // C304 buffer voltage (Y)
  double v_C306 = 0.0f;        // C306 buffer voltage (Z)
  double cc_C304 = 0.0;        // C304 charge constant (Y)
  double cc_C306 = 0.0;        // C306 charge constant (Z)
  double v_RAMP = 0.0;         // Simulated RAMP charge(*)
  double v_ZERO = 0.0;         // Simulated ZERO charge(*)
  double cc_RAMP = 0.0;        // Simulated RAMP charge constant(*)
  double cc_ZERO = 0.0;        // Simulated ZERO charge constant(*)
  double cc_C313 = 0.0;        // X integrator discharge constant
  double cc_C312 = 0.0;        // Y integrator discharge constant

  // Outputs
  double vX = 0.0f;           // X-axis output voltage
  double vY = 0.0f;           // Y-axis output voltage
  double vZ = 0.0f;           // Z-axis output voltage


  XYZAxisIntegrators(MPXPorts * mpx_, VIAPorts * via_, DACPorts * dac_)
      : mpx(mpx_), via(via_), dac(dac_) {
    updateConstants();
  }

  void setDt(float v) { dt = v; updateConstants(); }
  void setVdd(float v) { Vdd = v; }
  void setVss(float v) { Vss = v; }
  void setZeroX(float v) { v_r335 = v; }
  void setZeroY(float v) { v_r333 = v; }

  void updateConstants() {
    cc_C304 = std::exp((-dt) / (r_CD4052B * c_C304));
    cc_C306 = std::exp((-dt) / (r_CD4052B * c_C306));
    cc_RAMP = std::exp((-dt) / (r_RAMP * c_RAMP));
    cc_ZERO = std::exp((-dt) / (r_ZERO * c_ZERO));
    cc_C313 = std::exp((-dt) / (r_R320 * c_C313));
    cc_C312 = std::exp((-dt) / (r_R317 * c_C312));
  }

  void step() {

    // The Vectrex display driver consists of three different circuits, one for each of the
    // three available beam outputs (X,Y & Z). Two different sub-circuits are used in different
    // configurations for the three outputs according to the following table:
    //
    // X axis: An op-amp integrator.
    // Y axis: An op-amp buffer connected to an op-amp integrator.
    // Z axis: An op-amp buffer.
    //
    // The X and Y axis integrators can be configured in four different states with the help of
    // two analog switches, RAMP and ZERO according to the following table:
    //
    // 1. RAMP off, ZERO off
    // 2. RAMP off, ZERO on
    // 3. RAMP on,  ZERO off
    // 4. RAMP on,  ZERO on
    //
    // The Z axis, consisting only of a sample and hold buffer with the BLANK VIA output
    // connected to it, is the simplest axis from a functional standpoint.
    //
    // The X and Y axis' has a zero reference voltage divider connected to them via a
    // potentiometer circuit. When the ZERO switch is enabled, the integrator capacitor will
    // discharge to the voltage set by the potential coming from the potentiometer circuit.
    // There's also an offset voltage that can be fed to the + pin of the op-amp via the 1B
    // multiplexer output.
    //
    // Component impedance (from datasheets, might not be completely accurate):
    //
    //      CD4052B (multiplexer): 125 Ohm
    //      LF347N (op-amp): 200 Ohm
    //      MC1408P8 (DAC): 1K Ohm
    //      CD4066B (Switch): 125 Ohm
    //
    // Observations:
    //      The vectrex beam integrators are always integrating due to the zero ref voltage that
    //      feeds into the op-amp. Because the circuit is not ideal, there will always exist a
    //      voltage potential on the integrator -pin even if RAMP is OFF. This potential causes
    //      drifting as long as ZERO is OFF as well. If ZERO is on, the integrator output voltage
    //      is a combination of the zero ref potential and the integrator trying to integrate, but
    //      it will never be zero.
    //
    // Components: https://www.beardypig.com/assets/vectrex/images/vectrex_logic_board.png
    //

    // Y axis buffer
    if (!via->sh)
    {
      // The capacitor is charging to mpx->out1A...
      v_C304 += (mpx->out1A - v_C304) * (1.0f - cc_C304);
    }
    else
    {
      // The capacitor is decaying...
      v_C304 -= v_C304 * d_C304;
    }

    // Z axis buffer
    if (!via->sh)
    {
      // The capacitor is charging to mpx->out1C...
      v_C306 += (mpx->out1C - v_C306) * (1.0f - cc_C306);
    }
    else
    {
      // The capacitor is decaying...
      v_C306 -= v_C306 * d_C306;
    }

    // ------------------------------------------------------------------------------
    // BEGIN: ZERO & RAMP DELAY CALCULATIONS
    //

#if 0
    if (via->zero)
    {
      v_ZERO += (5.0f - v_ZERO) * (1.0f - cc_ZERO);
    }
    else
    {
      v_ZERO = v_ZERO * cc_ZERO;
    }

    if (via->ramp)
    {
      v_RAMP += (5.0f - v_RAMP) * (1.0f - cc_RAMP);
    }
    else
    {
      v_RAMP = v_RAMP * cc_RAMP;
    }
#endif

    bool ZERO = via->zero; //v_ZERO > (5.0f * d_ZERO);
    bool RAMP = via->ramp; //v_RAMP > (5.0f * d_RAMP);

    //
    // END: ZERO & RAMP DELAY CALCULATIONS
    // ------------------------------------------------------------------------------

    // Integrator input voltage variables
    double int_vX;
    double int_vY;

    // Integrator input resistance variables
    float r_intX;
    float r_intY;

    if (ZERO && RAMP) // ZERO = ON, RAMP = ON
    {
      // Integrator input voltage calculation
      int_vX = (dac->v * r_R332_334 + v_r335 * r_R316_319) / (r_R316_319 + r_R332_334);
      int_vY = (v_C304 * r_R332_334 + v_r333 * r_R316_319) / (r_R316_319 + r_R332_334);

      // Discharge the integrator capacitor(s)
      v_C313 = v_C313 * cc_C313;
      v_C312 = v_C312 * cc_C312;

      // Set integrator resistance's (parallel of R316_319 and R316_319)
      r_intX = (r_R316_319 * r_R332_334) / (r_R316_319 + r_R332_334);
      r_intY = (r_R316_319 * r_R332_334) / (r_R316_319 + r_R332_334);
    }
    else if (ZERO && !RAMP) // ZERO = ON, RAMP = OFF
    {
      // Integrator input voltage calculation is simply the zero reference voltage
      // coming from the potentiometer.
      int_vX = v_r335;
      int_vY = v_r333;

      // Integrator resistance is R332 & R334 (3.3M Ohm)
      r_intX = r_R332_334;
      r_intY = r_R332_334;

      // Discharge the integrator capacitor(s)
      v_C313 = v_C313 * cc_C313;
      v_C312 = v_C312 * cc_C312;
    }
    else if (!ZERO && RAMP) // ZERO = OFF, RAMP = ON
    {
      // Integrator input voltage calculation
      int_vX = (dac->v * r_R332_334 + v_r335 * r_R316_319) / (r_R316_319 + r_R332_334);
      int_vY = (v_C304 * r_R332_334 + v_r333 * r_R316_319) / (r_R316_319 + r_R332_334);

      // Set integrator resistance's (parallel of R316_319 and R316_319)
      r_intX = (r_R316_319 * r_R332_334) / (r_R316_319 + r_R332_334);
      r_intY = (r_R316_319 * r_R332_334) / (r_R316_319 + r_R332_334);
    }
    else if (!ZERO && !RAMP) // ZERO = OFF, RAMP = OFF
    {
      // In this mode, the beam will drift due to the integrator working against the output
      // voltage of the 0-ref potentiometer. In an ideal world the output from the 0-ref would
      // be zero (a virtual ground), but this is not the case in reality.

      // Integrator input voltage calculation is simply the zero reference voltage
      // coming from the potentiometer.
      int_vX = v_r335;
      int_vY = v_r333;

      // Integrator resistance is R332 & R334 (3.3M Ohm)
      r_intX = r_R332_334;
      r_intY = r_R332_334;
    }

    // Z-axis output; Either the voltage in v_C306, or 0v if BLANK is ON
    vZ = (via->blank) ? 0.0f : v_C306;

    // Intergration for X/Y.
    //
    // Vout = -(Vin / (C * R) * dt) + Vout;
    //
    // Vout is the voltage output from the integrator, but it's ALSO the value of the capacitor. This holds
    // only up until Vss is reached, the capacitor will actually continue to charge after the op-amp output
    // is saturated. It charges to the max output (Vss or Vdd) plus the input. The formula is output-input
    // since it's an inverting integrator: (+x - -y) or (-x - +y)
    //
    // Observations:
    //   Offset is added even if ZERO is ON

    // Y Integration
    {
      // Capacitor decay (leaks into the op amp)
      v_C312 -= v_C312 * d_C312;

      // New value after a single integration step
      auto integratorOut = -(int_vY / (c_C312 * r_intY) * dt) + v_C312;

      if (integratorOut < Vss || integratorOut > Vdd)
      {
        // Saturated output, clamp to Vss || Vdd
        integratorOut = clamp(integratorOut, (double)Vss, (double)Vdd);

        // Charge the capacitor to integratorOut (out) - int_vY (in)
        v_C312 += ((integratorOut - int_vY) - v_C312) * (1.0f - std::exp((-dt) / (r_intY * c_C312)));
      }
      else
      {
        // Non-saturated output, set the capacitor to the integrator output
        v_C312 = integratorOut;
      }

      // Final output with added offset
      vY = clamp(integratorOut + mpx->out1B, (double)Vss, (double)Vdd);
    }

    // X Integration
    {
      // Capacitor decay (leaks into the op amp)
      v_C313 -= v_C313 * d_C313;

      // New value after a single integration step
      auto integratorOut = -(int_vX / (c_C313 * r_intX) * dt) + v_C313;

      if (integratorOut < Vss || integratorOut > Vdd)
      {
        // Saturated output, clamp to Vss || Vdd
        integratorOut = clamp(integratorOut, (double)Vss, (double)Vdd);

        // Charge the capacitor to integratorOut (out) - int_vY (in)
        v_C313 += ((integratorOut - int_vX) - v_C313) * (1.0f - std::exp((-dt) / (r_intX * c_C313)));
      }
      else
      {
        // Non-saturated output, set the capacitor to the integrator output
        v_C313 = integratorOut;
      }

      // Final output with added offset
      vX = clamp(integratorOut + mpx->out1B, (double)Vss, (double)Vdd);
    }
  };
};
