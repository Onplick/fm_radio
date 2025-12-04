#include <benchmark/benchmark.h>
#include <random>
#include <vector>
#include <complex>
#include "dsp.hpp"


static std::vector<int16_t> make_iq_int16(size_t samples) {
    std::vector<int16_t> v(samples);
    std::mt19937 r(123);
    std::uniform_int_distribution<int16_t> d(-30000, 30000);
    for (auto &x : v) x = d(r);
    return v;
}

static std::vector<std::complex<float>> make_iq_f32(size_t samples) {
    std::vector<std::complex<float>> v(samples);
    std::mt19937 r(123);
    std::uniform_real_distribution<float> d(-1.0f, 1.0f);
    for (auto &x : v) x = {d(r), d(r)};
    return v;
}

static std::vector<float> make_audio(size_t samples) {
    std::vector<float> v(samples);
    std::mt19937 r(123);
    std::uniform_real_distribution<float> d(-0.5f, 0.5f);
    for (auto &x : v) x = d(r);
    return v;
}


static void BM_downsample_iq(benchmark::State& state) {
    const int decim = state.range(0);
    const size_t input_samples = 1 << 16;   // 65536 samples

    auto in = make_iq_int16(input_samples);
    std::vector<std::complex<float>> out;
    out.reserve(input_samples / decim + 1);

    for (auto _ : state) {
        benchmark::DoNotOptimize(in);
        benchmark::DoNotOptimize(out);

        dsp::downsample_iq(in, out, decim);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(int64_t(state.iterations()) * input_samples);
}
BENCHMARK(BM_downsample_iq)->Arg(2)->Arg(4)->Arg(8)->Arg(10)->Arg(16)->Unit(benchmark::kMicrosecond);

static void BM_demodulate_fm(benchmark::State& state) {
    const size_t N = state.range(0);
    auto in = make_iq_f32(N);
    std::vector<float> out;
    out.resize(N);

    dsp::DemodState st{};
    st.prev_iq = {1.0f, 0.0f};

    for (auto _ : state) {
        benchmark::DoNotOptimize(in);
        benchmark::DoNotOptimize(out);

        dsp::demodulate_fm(in, out, st);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations() * N);
}
BENCHMARK(BM_demodulate_fm)->Arg(4096)->Arg(16384)->Arg(65536)->Unit(benchmark::kMicrosecond);

static void BM_demodulate_am(benchmark::State& state) {
    const size_t N = state.range(0);
    auto in = make_iq_f32(N);
    std::vector<float> out;
    out.reserve(N);

    for (auto _ : state) {
        out.clear();

        benchmark::DoNotOptimize(in);
        benchmark::DoNotOptimize(out);

        dsp::demodulate_am(in, out);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations() * N);
}
BENCHMARK(BM_demodulate_am)->Arg(4096)->Arg(16384)->Arg(65536)->Unit(benchmark::kMicrosecond);

static void BM_downsample_audio(benchmark::State& state) {
    const int decim = state.range(0);
    const size_t N = 1 << 16;

    auto in = make_audio(N);
    std::vector<float> out;
    out.reserve(N / decim + 1);

    dsp::AudioDecimState st{};
    float gain = 1.0f;

    for (auto _ : state) {
        out.clear();
        st.accumulator = 0;
        st.counter = 0;

        dsp::downsample_audio(in, out, decim, st, gain);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations() * N);
}
BENCHMARK(BM_downsample_audio)->Arg(2)->Arg(4)->Arg(8)->Arg(16)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();