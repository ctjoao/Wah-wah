// Solo Wah - core DSP. Deliberately free of any JUCE dependency so it can be unit-tested on its own.
//
// Model: a wah pedal is a swept, resonant band-pass (the inductor/pot circuit) with some low-pass
// "body" underneath it. We use a TPT state-variable filter (Zavalishin) whose centre frequency is
// driven by the pedal position on an exponential (musical) scale.
#pragma once
#include <algorithm>
#include <cmath>
#include <complex>

namespace solowah
{
constexpr double kPi = 3.14159265358979323846;

struct Voicing
{
    const char* name;
    float fMin;       // Hz, pedal fully heel-down
    float fMax;       // Hz, pedal fully toe-down
    float q;          // resonance at the knob's centre position
    float lowBlend;   // amount of low-pass "body" mixed in (gives the heel position its thump)
    float peakGainDb; // level at the resonant peak
};

inline const Voicing& voicing (int index)
{
    static const Voicing v[] = {
        { "Classic",         450.0f, 1900.0f, 4.0f, 0.35f, 6.0f },
        { "Lead (KH-style)", 380.0f, 2300.0f, 5.5f, 0.30f, 8.0f },
        { "Vintage",         500.0f, 1500.0f, 3.0f, 0.50f, 5.0f },
    };
    return v[std::clamp (index, 0, 2)];
}

struct Settings
{
    float fc;    // centre frequency, Hz
    float q;     // resonance
    float peak;  // linear gain applied to the (unity-normalised) band-pass
    float low;   // linear gain of the low-pass body
};

// pos: 0 = heel down (dark), 1 = toe down (bright).  res: 0..1 (0.5 = voicing default).
// range: 0.5..1.5 scales the sweep width (in octaves) around its centre.
inline Settings settingsFor (int voicingIndex, float pos, float res, float range)
{
    const auto& vo = voicing (voicingIndex);
    const float lo = std::log (vo.fMin), hi = std::log (vo.fMax);
    const float centre = 0.5f * (lo + hi), width = hi - lo;

    Settings s;
    s.fc   = std::exp (centre + (pos - 0.5f) * width * range);
    s.q    = vo.q * std::pow (2.0f, (res - 0.5f) * 2.0f); // 0.5x .. 2x the voicing's Q
    s.q    = std::clamp (s.q, 1.0f, 14.0f);
    s.peak = std::pow (10.0f, vo.peakGainDb / 20.0f);
    s.low  = vo.lowBlend;
    return s;
}

// Magnitude of the filter's transfer function at frequency f (used by the GUI graph and the tests).
inline float magnitude (const Settings& s, double sampleRate, float f)
{
    const double g = std::tan (kPi * s.fc / sampleRate);
    const double w = std::tan (kPi * f / sampleRate) / g;
    const double k = 1.0 / s.q;
    const std::complex<double> sv (0.0, w);
    const auto den = sv * sv + k * sv + 1.0;
    const auto h = (double) s.peak * k * (sv / den) + (double) s.low * (1.0 / den);
    return (float) std::abs (h);
}

class WahFilter
{
public:
    void prepare (double sr, int numChannels)
    {
        sampleRate = sr;
        channels = std::min (numChannels, maxChannels);
        const auto coef = [sr] (double ms) { return 1.0f - (float) std::exp (-1.0 / (0.001 * ms * sr)); };
        aPedal = coef (6.0);   // fast enough to follow a rocking foot, slow enough to avoid zipper noise
        aOther = coef (20.0);
        reset();
        snapSmoothers = true;
    }

    void reset()
    {
        for (int c = 0; c < maxChannels; ++c)
            ic1[c] = ic2[c] = 0.0f;
    }

    void setParams (float pedal, int voicingIndex, float resonance, float range,
                    float drive, float mix, float outputDb)
    {
        tPedal = pedal;  tRes = resonance;  tRange = range;
        tDrive = drive;  tMix = mix;        tOut = std::pow (10.0f, outputDb / 20.0f);
        voice = voicingIndex;
        if (snapSmoothers)
        {
            sPedal = tPedal; sRes = tRes; sRange = tRange; sDrive = tDrive; sMix = tMix; sOut = tOut;
            snapSmoothers = false;
        }
    }

    void process (float* const* data, int numChannels, int numSamples)
    {
        const int nch = std::min (numChannels, channels);
        for (int i = 0; i < numSamples; ++i)
        {
            sPedal += aPedal * (tPedal - sPedal);
            sRes   += aOther * (tRes - sRes);
            sRange += aOther * (tRange - sRange);
            sDrive += aOther * (tDrive - sDrive);
            sMix   += aOther * (tMix - sMix);
            sOut   += aOther * (tOut - sOut);

            auto st = settingsFor (voice, sPedal, sRes, sRange);
            st.fc = std::min (st.fc, 0.45f * (float) sampleRate);

            const float g  = (float) std::tan (kPi * st.fc / sampleRate);
            const float k  = 1.0f / st.q;
            const float a1 = 1.0f / (1.0f + g * (g + k));
            const float a2 = g * a1;
            const float a3 = g * a2;

            const float driveGain = std::pow (10.0f, sDrive * 18.0f / 20.0f);
            const float driveComp = 1.0f / std::sqrt (driveGain);

            for (int c = 0; c < nch; ++c)
            {
                const float dry = data[c][i];
                const float in  = std::tanh (dry * driveGain) * driveComp;

                const float v3 = in - ic2[c];
                const float v1 = a1 * ic1[c] + a2 * v3;                 // band-pass (peak gain = Q)
                const float v2 = ic2[c] + a2 * ic1[c] + a3 * v3;        // low-pass
                ic1[c] = 2.0f * v1 - ic1[c];
                ic2[c] = 2.0f * v2 - ic2[c];

                const float wet = st.peak * k * v1 + st.low * v2;
                data[c][i] = (dry * (1.0f - sMix) + wet * sMix) * sOut;
            }
        }
    }

private:
    static constexpr int maxChannels = 2;
    double sampleRate = 44100.0;
    int channels = 2, voice = 1;
    float aPedal = 0.01f, aOther = 0.005f;
    float tPedal = 0.5f, tRes = 0.5f, tRange = 1.0f, tDrive = 0.0f, tMix = 1.0f, tOut = 1.0f;
    float sPedal = 0.5f, sRes = 0.5f, sRange = 1.0f, sDrive = 0.0f, sMix = 1.0f, sOut = 1.0f;
    float ic1[maxChannels] {}, ic2[maxChannels] {};
    bool snapSmoothers = true;
};
} // namespace solowah
