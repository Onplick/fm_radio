#pragma once

#include <complex>
#include <span>
#include <vector>

namespace dsp {

struct DemodState {
    std::complex<float> prev_iq = {0.0f, 0.0f};
};

struct AudioDecimState {
    float accumulator = 0.0f;
    int   counter     = 0;
};

void downsample_iq(std::span<const int16_t> input,
                   std::vector<std::complex<float>>& output,
                   int decimation);

void demodulate(std::span<const std::complex<float>> input,
                std::vector<float>& output,
                DemodState& state);

void downsample_audio(std::span<const float> input,
                      std::vector<float>& output,
                      int decimation,
                      AudioDecimState& state);

} // namespace dsp
