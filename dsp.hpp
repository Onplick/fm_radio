#pragma once

#include <complex>
#include <span>
#include <vector>

/**
 * @file dsp.hpp
 * @brief Digital Signal Processing (DSP) primitives for FM demodulation,
 *        decimation, and audio downsampling.
 */

namespace dsp {

/**
 * @brief Stateful information required for continuous FM demodulation.
 *
 * FM demodulation relies on computing the phase difference between consecutive
 * complex IQ samples. To support block-based processing (buffers processed one
 * after another), the previous IQ sample must be preserved between calls.
 *
 * @note This state must be maintained per channel/stream.
 */
struct DemodState {
    /// Last IQ sample of previous block (required for phase continuity)
    std::complex<float> prev_iq = {0.0f, 0.0f};
};

/**
 * @brief Stateful accumulator for block-based audio decimation.
 *
 * Audio downsampling usually computes an average over N input samples.
 * To support chunk-based processing, partial accumulation must be preserved
 * between calls.
 */
struct AudioDecimState {
    /// Accumulated sum of values in the current decimation window
    float accumulator = 0.0f;

    /// Number of samples accumulated so far
    int counter     = 0;
};

/**
 * @brief Downsample interleaved IQ samples using simple boxcar averaging.
 *
 * Input format:
 * @code
 *   [I0, Q0, I1, Q1, I2, Q2, ...]  (int16_t interleaved)
 * @endcode
 *
 * The function sums `decimation` consecutive IQ samples and produces one
 * complex output sample representing their average magnitude (division by N
 * is intentionally omitted; downstream demodulation is phase-only).
 *
 * SIMD acceleration may be applied automatically at compile time
 *
 * @param input         Interleaved I/Q input samples as a span of int16_t.
 * @param output        Vector to store downsampled complex<float> IQ samples.
 * @param decimation    Number of IQ pairs to combine into one output sample.
 *
 * @note Output vector is cleared and resized appropriately.
 */
void downsample_iq(std::span<const int16_t> input,
                   std::vector<std::complex<float>>& output,
                   int decimation);

/**
 * @brief Perform FM demodulation (phase differencing) on complex IQ data.
 *
 * FM demodulation computes:
 * @f[
 *     \Delta\phi = \arg(x[n] \cdot x^*[n-1])
 * @f]
 *
 * This returns the instantaneous frequency deviation, which corresponds to
 * the FM-modulated audio waveform.
 *
 * @param input     Span of complex IQ samples after downsampling.
 * @param output    Vector to receive demodulated floating point samples.
 * @param state     DemodState containing the previous IQ sample needed
 *                  for continuous phase demodulation across block boundaries.
 *
 * @note Output vector is resized to match input size.
 */
void demodulate(std::span<const std::complex<float>> input,
                std::vector<float>& output,
                DemodState& state);

/**
 * @brief Downsample audio via simple decimation averaging.
 *
 * This implements:
 *
 * @code
 *   y[n] = (x[k] + x[k+1] + ... + x[k+decimation-1]) * (gain / decimation)
 * @endcode
 *
 * @param input         FM demodulated samples (float).
 * @param output        Output audio samples after decimation.
 * @param decimation    Number of input samples per output sample.
 * @param state         Stateful accumulation for block-based processing.
 * @param gain          Optional gain applied to output audio.
 *
 * @note Output vector is cleared before writing results.
 * @note Gain gives control over volume
 */
void downsample_audio(std::span<const float> input,
                      std::vector<float>& output,
                      int decimation,
                      AudioDecimState& state);

} // namespace dsp
