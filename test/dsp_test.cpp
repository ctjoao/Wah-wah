// Standalone check of the wah DSP: sweep, resonance, stability, analytic-vs-measured response.
#include "../Source/WahDSP.h"
#include <cstdio>
#include <vector>

using namespace solowah;

static double measureGainDb (int voicingIndex, float pedal, float freq, double fs)
{
    WahFilter f;
    f.prepare (fs, 1);
    f.setParams (pedal, voicingIndex, 0.5f, 1.0f, 0.0f, 1.0f, 0.0f);
    const int n = (int) fs; // 1 second
    std::vector<float> x ((size_t) n);
    for (int i = 0; i < n; ++i) x[(size_t) i] = 0.1f * (float) std::sin (2.0 * kPi * freq * i / fs);
    float* ch[1] = { x.data() };
    f.process (ch, 1, n);
    double acc = 0; int cnt = 0;
    for (int i = n / 2; i < n; ++i) { acc += (double) x[(size_t) i] * x[(size_t) i]; ++cnt; }
    return 10.0 * std::log10 (acc / cnt / (0.1 * 0.1 / 2.0));
}

int main()
{
    const double fs = 48000.0;
    int fails = 0;
    auto check = [&] (bool ok, const char* what) { std::printf ("%-62s %s\n", what, ok ? "PASS" : "FAIL"); fails += ! ok; };

    // 1. Pedal sweeps the resonant peak from heel to toe, monotonically.
    for (int v = 0; v < 3; ++v)
    {
        float prev = 0; bool mono = true;
        std::printf ("%s: ", voicing (v).name);
        for (float p = 0.0f; p <= 1.0f; p += 0.25f)
        {
            const auto s = settingsFor (v, p, 0.5f, 1.0f);
            std::printf ("%.0fHz ", s.fc);
            mono &= (s.fc > prev); prev = s.fc;
        }
        std::printf ("\n");
        check (mono, "centre frequency rises monotonically with pedal");
        check (std::fabs (settingsFor (v, 0, 0.5f, 1).fc - voicing (v).fMin) < 1.0f
            && std::fabs (settingsFor (v, 1, 0.5f, 1).fc - voicing (v).fMax) < 1.0f, "heel/toe hit fMin/fMax");
    }

    // 2. Measured response of the real-time filter matches the analytic curve used by the GUI.
    {
        const float pos = 0.7f;
        const auto s = settingsFor (1, pos, 0.5f, 1.0f);
        double maxErr = 0;
        for (float f : { 200.f, 400.f, 800.f, 1200.f, 1700.f, 2500.f, 5000.f })
        {
            const double meas = measureGainDb (1, pos, f, fs);
            const double ana  = 20.0 * std::log10 (magnitude (s, fs, f));
            maxErr = std::max (maxErr, std::fabs (meas - ana));
            std::printf ("  %5.0f Hz  measured %+6.2f dB  analytic %+6.2f dB\n", f, meas, ana);
        }
        check (maxErr < 0.3, "measured vs analytic response within 0.3 dB");
    }

    // 3. Peak is at fc and clearly higher than the response an octave+ away.
    {
        const auto s = settingsFor (1, 0.5f, 0.5f, 1.0f);
        const float atPeak = magnitude (s, fs, s.fc), below = magnitude (s, fs, s.fc / 4), above = magnitude (s, fs, s.fc * 4);
        std::printf ("  peak %.1f dB, -2oct %.1f dB, +2oct %.1f dB\n",
                     20 * std::log10 (atPeak), 20 * std::log10 (below), 20 * std::log10 (above));
        check (atPeak > below * 3.0f && atPeak > above * 3.0f, "resonant peak stands >9.5 dB above skirts");
    }

    // 4. Stability: loud noise, extreme settings, fast pedal flapping -> finite and bounded.
    {
        WahFilter f; f.prepare (fs, 2);
        const int n = 48000 * 5;
        std::vector<float> l ((size_t) n), r ((size_t) n);
        unsigned seed = 1;
        for (int i = 0; i < n; ++i) { seed = seed * 1664525u + 1013904223u; l[(size_t) i] = ((seed >> 8) / 8388608.0f - 1.0f); r[(size_t) i] = -l[(size_t) i]; }
        float* ch[2] = { l.data(), r.data() };
        bool finite = true; float peak = 0;
        for (int b = 0; b < n; b += 128)
        {
            const float p = 0.5f + 0.5f * (float) std::sin (2.0 * kPi * 6.0 * b / fs); // 6 Hz rocking
            f.setParams (p, 1, 1.0f, 1.5f, 1.0f, 1.0f, 12.0f);
            float* blk[2] = { ch[0] + b, ch[1] + b };
            f.process (blk, 2, std::min (128, n - b));
        }
        for (int i = 0; i < n; ++i) { finite &= std::isfinite (l[(size_t) i]) && std::isfinite (r[(size_t) i]); peak = std::max (peak, std::fabs (l[(size_t) i])); }
        std::printf ("  worst-case peak output %.2f\n", peak);
        check (finite && peak < 50.0f, "stable under extreme settings + 6 Hz pedal rocking");
    }

    // 5. Mix = 0 and output 0 dB is a bit-transparent bypass (after smoothing settles).
    {
        WahFilter f; f.prepare (fs, 1);
        f.setParams (0.5f, 1, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f);
        std::vector<float> x (1000, 0.25f); float* ch[1] = { x.data() };
        f.process (ch, 1, 1000);
        check (std::fabs (x[999] - 0.25f) < 1e-6f, "mix=0 passes dry signal unchanged");
    }

    std::printf (fails ? "\n%d FAILURE(S)\n" : "\nAll DSP checks passed\n", fails);
    return fails ? 1 : 0;
}
