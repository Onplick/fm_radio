#include "dsp.hpp"

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#endif

namespace dsp {

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
static inline int32_t horizontal_sum_8(int16x8_t v) {
    int16x4_t sum4 = vpadd_s16(vget_low_s16(v), vget_high_s16(v));
    int16x4_t sum2 = vpadd_s16(sum4, sum4);
    return static_cast<int32_t>(vget_lane_s16(sum2, 0));
}
#endif

void downsample_iq(std::span<const int16_t> in,
                   std::vector<std::complex<float>>& out,
                   int decim)
{
    constexpr int kIQComponents = 2;
    const int stride = decim * kIQComponents;

    out.clear();

    if (in.size() < static_cast<size_t>(stride))
        return;

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    {
        size_t total_pairs = in.size() / 2;
        size_t blocks = total_pairs / decim;

        out.reserve(out.size() + blocks);

        const int16_t* ptr = in.data();
        const int16_t* endptr = ptr + blocks * stride;

        while (ptr < endptr) {
            // Load 8 IQ pairs
            int16x8x2_t iq = vld2q_s16(ptr);

            int32_t si = horizontal_sum_8(iq.val[0]);
            int32_t sq = horizontal_sum_8(iq.val[1]);

            // Remaining 2 pairs
            const int16_t* tail = ptr + 16;
            si += tail[0] + tail[2];
            sq += tail[1] + tail[3];

            out.emplace_back(static_cast<float>(si),
                             static_cast<float>(sq));

            ptr += stride;
        }
    }
#else //__ARM_NEON || __ARM_NEON__

    const size_t n = in.size();
    out.reserve(out.size() + n / stride + 1);

    for (size_t i = 0; i + stride <= n; i += stride) {
        float si = 0.f, sq = 0.f;
        for (size_t k = 0; k < stride; k += 2) {
            si += in[i + k];
            sq += in[i + k + 1];
        }

        out.emplace_back(si, sq);
    }
#endif //__ARM_NEON || __ARM_NEON__
}

void demodulate_fm(std::span<const std::complex<float>> in,
                std::vector<float>& out,
                DemodState& state)
{
    out.resize(in.size());
    auto out_it = out.begin();

    for (const auto& sample : in) {
        const auto prod = sample * std::conj(state.prev_iq);
        *out_it++ = std::atan2(prod.imag(), prod.real());
        state.prev_iq = sample;
    }
}

void demodulate_am(std::span<const std::complex<float>> in,
                   std::vector<float>& out)
{
    out.clear();
    out.reserve(in.size());

#if defined(__ARM_NEON__)
    const float* ptr = reinterpret_cast<const float*>(in.data());
    size_t count = in.size();
    size_t i = 0;

    // Process 4 complex samples (8 floats) at a time
    for (; i + 4 <= count; i += 4) {
        float32x4x2_t iq = vld2q_f32(ptr);  // loads I=iq.val[0], Q=iq.val[1]

        float32x4_t i2 = vmulq_f32(iq.val[0], iq.val[0]);
        float32x4_t q2 = vmulq_f32(iq.val[1], iq.val[1]);

        float32x4_t sum = vaddq_f32(i2, q2);
        float32x4_t mag = vsqrtq_f32(sum);

        vst1q_f32(&out.emplace_back(), mag);

        ptr += 8; // 8 floats per 4 complex samples
    }

    // Process leftovers
    for (; i < count; i++) {
        out.push_back(std::abs(in[i]));
    }

#else
    for (auto& s : in) {
        out.push_back(std::abs(s));
    }
#endif
}

void downsample_audio(std::span<const float> in,
                      std::vector<float>& out,
                      int decim,
                      AudioDecimState& state,
                      float gain)
{
    const float scale = gain / static_cast<float>(decim);
  
    out.clear();

    for (float v : in) {
        state.accumulator += v;

        if (++state.counter == decim) {
            out.push_back(state.accumulator * scale);
            state.accumulator = 0.0f;
            state.counter = 0;
        }
    }
}

} // namespace dsp
