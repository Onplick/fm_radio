#pragma once

#include <complex>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <iio.h>

#include "dsp.hpp"
#include "udp_sender.hpp"

/**
 * @file plutosdr.hpp
 * @brief PlutoSDR FM receiver integrating libIIO and DSP pipeline
 */

/// RAII deleter for iio_context
struct ContextDeleter {
    void operator()(iio_context* ctx) const noexcept;
};

/// RAII deleter for iio_buffer
struct BufferDeleter {
    void operator()(iio_buffer* buf) const noexcept;
};

using ContextPtr = std::unique_ptr<iio_context, ContextDeleter>;
using BufferPtr  = std::unique_ptr<iio_buffer,  BufferDeleter>;

/**
 * @class PlutoSDR
 * @brief Handles PlutoSDR hardware, IQ acquisition, DSP chain, and audio output
 */
class PlutoSDR {
public:
    /**
     * @brief Construct PlutoSDR receiver
     * @param frequency_hz Center RF frequency (Hz)
     * @param gain_db      RF gain (dB)
     * @param udp_ip       Optional UDP destination IP
     * @param udp_port     Optional UDP port
     * @param audio_gain   Audio gain applied after DSP
     */
    PlutoSDR(long long frequency_hz,
             double gain_db,
             std::optional<std::string> udp_ip = std::nullopt,
             std::optional<int> udp_port       = std::nullopt,
             float audio_gain                  = 0.3f);

    /// Start continuous receive nad output loop
    void run();

private:
    // Config
    static constexpr const char* kPlutoUri    = "ip:pluto.local";
    static constexpr long long kInputRateHz = 2'400'000;   // 2.4 MSPS
    static constexpr std::size_t kBufferSize  = 120'000;     // 50 ms
    static constexpr int kDecimIq     = 10;          // 2.4M -> 240k
    static constexpr int kDecimAudio  = 5;           // 240k -> 48k

    // User parameters
    long long frequency_hz_;
    double gain_db_;
    float audio_gain_;

    // IIO device
    ContextPtr ctx_;
    iio_device*  dev_rx_ = nullptr;
    iio_channel* rx_chan_i_ = nullptr;
    BufferPtr rx_buffer_;

    // DSP buffers and state
    dsp::DemodState demod_state_;
    dsp::AudioDecimState audio_state_;
    std::vector<std::complex<float>> iq_buf_;
    std::vector<float> freq_buf_;

    // Optional UDP output
    UdpSender udp_;
    bool use_udp_ = false;

    /// Configure PlutoSDR hardware (frequency, gain, sampling rate).
    void initialize_hardware();

    /// Process a block of raw I/Q samples through DSP pipeline.
    void process_block(std::span<const int16_t> raw, std::vector<float>& audio_out);

    /// Output audio either to UDP or stdout.
    void output_audio(const std::vector<float>& audio);
};
