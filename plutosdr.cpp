#include "plutosdr.hpp"
#include <cstring>
#include <stdexcept>
#include <iostream>

void ContextDeleter::operator()(iio_context* ctx) const noexcept {
    if (ctx) iio_context_destroy(ctx);
}

void BufferDeleter::operator()(iio_buffer* buf) const noexcept {
    if (buf) iio_buffer_destroy(buf);
}

namespace {

/// Simple helper for writing numeric IIO attributes
inline void write_attr(iio_channel* ch, const char* name, long long value) {
    if (!ch || iio_channel_attr_write_longlong(ch, name, value) < 0)
        throw std::runtime_error(std::string("Failed attr: ") + name);
}

/// Simple helper for writing string IIO attributes
inline void write_attr(iio_channel* ch, const char* name, const char* value) {
    if (!ch || iio_channel_attr_write(ch, name, value) < 0)
        throw std::runtime_error(std::string("Failed attr: ") + name);
}

} // namespace

PlutoSDR::PlutoSDR(long long frequency_hz,
                   double gain_db,
                   std::optional<std::string> udp_ip,
                   std::optional<int> udp_port,
                   float audio_gain)
    : frequency_hz_{frequency_hz}
    , gain_db_{gain_db}
    , audio_gain_{audio_gain}
    , iq_buf_(kBufferSize / kDecimIq + 64)
    , freq_buf_(kBufferSize / kDecimIq + 64)
{
    if (udp_ip.has_value() && udp_port.has_value()) {
        udp_.open(udp_ip.value(), udp_port.value());
        use_udp_ = true;
    }

    initialize_hardware();
}

void PlutoSDR::initialize_hardware()
{
    ctx_.reset(iio_create_context_from_uri(PlutoConfig::kPlutoUri));
    if (!ctx_)
        throw std::runtime_error("Failed to create IIO context");

    auto* phy   = iio_context_find_device(ctx_.get(), PlutoConfig::kDevicePhy);
    dev_rx_     = iio_context_find_device(ctx_.get(), PlutoConfig::kDeviceRx);
    if (!phy || !dev_rx_)
        throw std::runtime_error("PlutoSDR devices not found");

    auto* lo = iio_device_find_channel(phy, PlutoConfig::kChannelLO, true);
    auto* rf = iio_device_find_channel(phy, PlutoConfig::kChannelRxI, false);

    write_attr(lo, PlutoConfig::kAttrFrequency, frequency_hz_);
    write_attr(rf, PlutoConfig::kAttrSampleRate, PlutoConfig::kInputRateHz);
    write_attr(rf, PlutoConfig::kAttrGainMode,   PlutoConfig::kGainModeManual);
    write_attr(rf, PlutoConfig::kAttrGain,       static_cast<long long>(gain_db_));

    rx_chan_i_ = iio_device_find_channel(dev_rx_,
                                         PlutoConfig::kChannelRxI, false);
    auto* rx_q = iio_device_find_channel(dev_rx_,
                                         PlutoConfig::kChannelRxQ, false);

    iio_channel_enable(rx_chan_i_);
    iio_channel_enable(rx_q);

    rx_buffer_.reset(iio_device_create_buffer(dev_rx_,
                                              PlutoConfig::kBufferSize,
                                              false));
    if (!rx_buffer_)
        throw std::runtime_error("Failed to create RX buffer");
}

void PlutoSDR::process_block(std::span<const int16_t> raw, std::vector<float>& audio_out)
{
    dsp::downsample_iq(raw, iq_buf_, kDecimIq);
    dsp::demodulate(iq_buf_, freq_buf_, demod_state_);
    dsp::downsample_audio(freq_buf_, audio_out, kDecimAudio, audio_state_, audio_gain_);
}

void PlutoSDR::output_audio(const std::vector<float>& audio)
{
    if (audio.empty()) return;

    if (use_udp_ && udp_.is_open()) {
        udp_.send(audio);
    } else {
        std::cout.write(reinterpret_cast<const char*>(audio.data()),
                        audio.size() * sizeof(float));
    }
}

void PlutoSDR::run()
{
    std::vector<float> audio_out;
    audio_out.reserve(PlutoConfig::kBufferSize /
                      (PlutoConfig::kDecimIq * PlutoConfig::kDecimAudio) + 64);

    while (true) {
        if (iio_buffer_refill(rx_buffer_.get()) < 0)
            break;

        auto* start = static_cast<int16_t*>(
            iio_buffer_first(rx_buffer_.get(), rx_chan_i_));

        auto* end = static_cast<int16_t*>(
            iio_buffer_end(rx_buffer_.get()));

        std::span<const int16_t> raw{start, static_cast<size_t>(end - start)};

        process_block(raw, audio_out);
        output_audio(audio_out);
    }
}
