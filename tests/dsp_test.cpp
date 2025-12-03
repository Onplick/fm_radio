#include <gtest/gtest.h>
#include "dsp.hpp"
#include <cmath>
#include <numbers>

using namespace dsp;

// Helper function to compare floats with tolerance
bool approx_equal(float a, float b, float epsilon = 1e-5f) {
    return std::abs(a - b) < epsilon;
}

bool approx_equal(const std::complex<float>& a, const std::complex<float>& b, float epsilon = 1e-5f) {
    return approx_equal(a.real(), b.real(), epsilon) && 
           approx_equal(a.imag(), b.imag(), epsilon);
}

TEST(DownsampleIQTest, EmptyInput) {
    std::vector<int16_t> in;
    std::vector<std::complex<float>> out;
    
    downsample_iq(in, out, 2);
    
    EXPECT_TRUE(out.empty());
}

TEST(DownsampleIQTest, InsufficientData) {
    std::vector<int16_t> in = {1, 2, 3};  // Less than stride
    std::vector<std::complex<float>> out;
    
    downsample_iq(in, out, 2);
    
    EXPECT_TRUE(out.empty());
}

TEST(DownsampleIQTest, SimpleDecimation) {
    // I: 1, 3, 5, 7  Q: 2, 4, 6, 8
    std::vector<int16_t> in = {1, 2, 3, 4, 5, 6, 7, 8};
    std::vector<std::complex<float>> out;
    
    downsample_iq(in, out, 2);  // decim=2 means sum 2 IQ pairs
    
    ASSERT_EQ(out.size(), 2);
    EXPECT_TRUE(approx_equal(out[0], std::complex<float>(1+3, 2+4)));  // (4, 6)
    EXPECT_TRUE(approx_equal(out[1], std::complex<float>(5+7, 6+8)));  // (12, 14)
}

TEST(DownsampleIQTest, Decimation10) {
    std::vector<int16_t> in(20, 1);  // 10 IQ pairs, all ones
    std::vector<std::complex<float>> out;
    
    downsample_iq(in, out, 10);
    
    ASSERT_EQ(out.size(), 1);
    EXPECT_TRUE(approx_equal(out[0], std::complex<float>(10.0f, 10.0f)));
}

TEST(DownsampleIQTest, Decimation5) {
    // Test decim=5 (uses partial blocks)
    std::vector<int16_t> in(10, 2);  // 5 IQ pairs, all twos
    std::vector<std::complex<float>> out;
    
    downsample_iq(in, out, 5);
    
    ASSERT_EQ(out.size(), 1);
    EXPECT_TRUE(approx_equal(out[0], std::complex<float>(10.0f, 10.0f)));
}

TEST(DownsampleIQTest, Decimation16) {
    // Test decim=16 (8 pairs + 8 pairs)
    std::vector<int16_t> in(32, 1);  // 16 IQ pairs, all ones
    std::vector<std::complex<float>> out;
    
    downsample_iq(in, out, 16);
    
    ASSERT_EQ(out.size(), 1);
    EXPECT_TRUE(approx_equal(out[0], std::complex<float>(16.0f, 16.0f)));
}

TEST(DownsampleIQTest, Decimation20) {
    // Test decim=20 (8+8+4 pairs)
    std::vector<int16_t> in(40, 3);  // 20 IQ pairs, all threes
    std::vector<std::complex<float>> out;
    
    downsample_iq(in, out, 20);
    
    ASSERT_EQ(out.size(), 1);
    EXPECT_TRUE(approx_equal(out[0], std::complex<float>(60.0f, 60.0f)));
}

TEST(DownsampleIQTest, VariedValuesDecim8) {
    // Test with varied values to ensure proper summing
    // I values: 0,1,2,3,4,5,6,7  Q values: 10,11,12,13,14,15,16,17
    std::vector<int16_t> in;
    for (int i = 0; i < 8; i++) {
        in.push_back(i);           // I
        in.push_back(10 + i);      // Q
    }
    
    std::vector<std::complex<float>> out;
    downsample_iq(in, out, 8);
    
    ASSERT_EQ(out.size(), 1);
    float expected_i = 0+1+2+3+4+5+6+7;  // 28
    float expected_q = 10+11+12+13+14+15+16+17;  // 108
    EXPECT_TRUE(approx_equal(out[0], std::complex<float>(expected_i, expected_q)));
}

TEST(DownsampleIQTest, VariedValuesDecim9) {
    // Test decim=9 (8 pairs + 1 pair scalar)
    std::vector<int16_t> in;
    for (int i = 0; i < 9; i++) {
        in.push_back(i);           // I: 0..8
        in.push_back(100 + i);     // Q: 100..108
    }
    
    std::vector<std::complex<float>> out;
    downsample_iq(in, out, 9);
    
    ASSERT_EQ(out.size(), 1);
    float expected_i = 0+1+2+3+4+5+6+7+8;  // 36
    float expected_q = 100+101+102+103+104+105+106+107+108;  // 936
    EXPECT_TRUE(approx_equal(out[0], std::complex<float>(expected_i, expected_q)));
}

TEST(DownsampleIQTest, VariedValuesDecim17) {
    // Test decim=17 (8+8+1 pairs)
    std::vector<int16_t> in;
    for (int i = 0; i < 17; i++) {
        in.push_back(i);           // I: 0..16
        in.push_back(50 + i);      // Q: 50..66
    }
    
    std::vector<std::complex<float>> out;
    downsample_iq(in, out, 17);
    
    ASSERT_EQ(out.size(), 1);
    float expected_i = 0+1+2+3+4+5+6+7+8+9+10+11+12+13+14+15+16;  // 136
    float expected_q = 0;
    for (int i = 50; i <= 66; i++) expected_q += i;  // 986
    EXPECT_TRUE(approx_equal(out[0], std::complex<float>(expected_i, expected_q)));
}

TEST(DownsampleIQTest, MultipleBlocksVariedDecim8) {
    // Test multiple blocks with decim=8
    std::vector<int16_t> in;
    
    // Block 1: I=0..7, Q=100..107
    for (int i = 0; i < 8; i++) {
        in.push_back(i);
        in.push_back(100 + i);
    }
    
    // Block 2: I=10..17, Q=200..207
    for (int i = 10; i < 18; i++) {
        in.push_back(i);
        in.push_back(200 + (i-10));
    }
    
    std::vector<std::complex<float>> out;
    downsample_iq(in, out, 8);
    
    ASSERT_EQ(out.size(), 2);
    
    // Block 1 sums
    float block1_i = 0+1+2+3+4+5+6+7;  // 28
    float block1_q = 100+101+102+103+104+105+106+107;  // 828
    EXPECT_TRUE(approx_equal(out[0], std::complex<float>(block1_i, block1_q)));
    
    // Block 2 sums
    float block2_i = 10+11+12+13+14+15+16+17;  // 108
    float block2_q = 200+201+202+203+204+205+206+207;  // 1628
    EXPECT_TRUE(approx_equal(out[1], std::complex<float>(block2_i, block2_q)));
}

TEST(DownsampleIQTest, MultipleBlocksVariedDecim10) {
    // Test multiple blocks with decim=10
    std::vector<int16_t> in;
    
    // Block 1: I=1..10, Q=201..210
    for (int i = 1; i <= 10; i++) {
        in.push_back(i);
        in.push_back(200 + i);
    }
    
    // Block 2: I=11..20, Q=211..220
    for (int i = 11; i <= 20; i++) {
        in.push_back(i);
        in.push_back(200 + i);
    }
    
    // Block 3: I=21..30, Q=221..230
    for (int i = 21; i <= 30; i++) {
        in.push_back(i);
        in.push_back(200 + i);
    }
    
    std::vector<std::complex<float>> out;
    downsample_iq(in, out, 10);
    
    ASSERT_EQ(out.size(), 3);
    
    // Block 1
    float block1_i = 1+2+3+4+5+6+7+8+9+10;  // 55
    float block1_q = 201+202+203+204+205+206+207+208+209+210;  // 2055
    EXPECT_TRUE(approx_equal(out[0], std::complex<float>(block1_i, block1_q)));
    
    // Block 2
    float block2_i = 11+12+13+14+15+16+17+18+19+20;  // 155
    float block2_q = 211+212+213+214+215+216+217+218+219+220;  // 2155
    EXPECT_TRUE(approx_equal(out[1], std::complex<float>(block2_i, block2_q)));
    
    // Block 3
    float block3_i = 21+22+23+24+25+26+27+28+29+30;  // 255
    float block3_q = 221+222+223+224+225+226+227+228+229+230;  // 2255
    EXPECT_TRUE(approx_equal(out[2], std::complex<float>(block3_i, block3_q)));
}

TEST(DownsampleIQTest, NegativeValues) {
    // Test with negative values (typical in IQ data)
    std::vector<int16_t> in = {
        -100, 50,   // pair 1
        100, -50,   // pair 2
        -80, 80,    // pair 3
        80, -80,    // pair 4
        -60, 60,    // pair 5
        60, -60,    // pair 6
        -40, 40,    // pair 7
        40, -40     // pair 8
    };
    
    std::vector<std::complex<float>> out;
    downsample_iq(in, out, 8);
    
    ASSERT_EQ(out.size(), 1);
    float expected_i = -100+100-80+80-60+60-40+40;  // 0
    float expected_q = 50-50+80-80+60-60+40-40;     // 0
    EXPECT_TRUE(approx_equal(out[0], std::complex<float>(expected_i, expected_q)));
}

TEST(DownsampleIQTest, MaxInt16Values) {
    // Test with maximum int16 values to check for overflow
    std::vector<int16_t> in(16, 32767);  // 8 IQ pairs, max int16 value
    std::vector<std::complex<float>> out;
    
    downsample_iq(in, out, 8);
    
    ASSERT_EQ(out.size(), 1);
    float expected = 32767.0f * 8;
    EXPECT_TRUE(approx_equal(out[0].real(), expected, 1.0f)) 
        << "Expected I: " << expected << ", Got: " << out[0].real();
    EXPECT_TRUE(approx_equal(out[0].imag(), expected, 1.0f))
        << "Expected Q: " << expected << ", Got: " << out[0].imag();
}

TEST(DownsampleIQTest, MinInt16Values) {
    // Test with minimum int16 values
    std::vector<int16_t> in(16, -32768);  // 8 IQ pairs, min int16 value
    std::vector<std::complex<float>> out;
    
    downsample_iq(in, out, 8);
    
    ASSERT_EQ(out.size(), 1);
    float expected = -32768.0f * 8;
    EXPECT_TRUE(approx_equal(out[0].real(), expected, 1.0f))
        << "Expected I: " << expected << ", Got: " << out[0].real();
    EXPECT_TRUE(approx_equal(out[0].imag(), expected, 1.0f))
        << "Expected Q: " << expected << ", Got: " << out[0].imag();
}

TEST(DownsampleIQTest, Decim3Scalar) {
    // Test small decimation
    std::vector<int16_t> in = {1, 2, 3, 4, 5, 6};  // 3 IQ pairs
    std::vector<std::complex<float>> out;
    
    downsample_iq(in, out, 3);
    
    ASSERT_EQ(out.size(), 1);
    EXPECT_TRUE(approx_equal(out[0], std::complex<float>(1+3+5, 2+4+6)));  // (9, 12)
}

TEST(DownsampleIQTest, Decim7Scalar) {
    // Test decim=7 (less than 8, uses scalar for remainder)
    std::vector<int16_t> in;
    for (int i = 0; i < 7; i++) {
        in.push_back(i);
        in.push_back(i * 10);
    }
    
    std::vector<std::complex<float>> out;
    downsample_iq(in, out, 7);
    
    ASSERT_EQ(out.size(), 1);
    float expected_i = 0+1+2+3+4+5+6;  // 21
    float expected_q = 0+10+20+30+40+50+60;  // 210
    EXPECT_TRUE(approx_equal(out[0], std::complex<float>(expected_i, expected_q)));
}

TEST(DownsampleIQTest, PartialBlockAtEnd) {
    // Test that partial blocks at the end are ignored
    std::vector<int16_t> in;
    
    // Full block: 8 IQ pairs
    for (int i = 0; i < 8; i++) {
        in.push_back(1);
        in.push_back(2);
    }
    
    // Partial data (only 3 IQ pairs, not enough for decim=8)
    in.push_back(99);
    in.push_back(99);
    in.push_back(99);
    in.push_back(99);
    in.push_back(99);
    in.push_back(99);
    
    std::vector<std::complex<float>> out;
    downsample_iq(in, out, 8);
    
    ASSERT_EQ(out.size(), 1);  // Only one complete block
    EXPECT_TRUE(approx_equal(out[0], std::complex<float>(8.0f, 16.0f)));
}

TEST(DownsampleIQTest, ConsistencyAcrossDecimFactors) {
    // Verify that summing works correctly regardless of block boundaries
    std::vector<int16_t> base_data;
    for (int i = 0; i < 24; i++) {
        base_data.push_back(i);
        base_data.push_back(i + 100);
    }
    
    // Test decim=8 (24 pairs = 3 complete blocks)
    std::vector<std::complex<float>> out8;
    downsample_iq(base_data, out8, 8);
    ASSERT_EQ(out8.size(), 3);
    
    // Test decim=12 (24 pairs = 2 complete blocks)
    std::vector<std::complex<float>> out12;
    downsample_iq(base_data, out12, 12);
    ASSERT_EQ(out12.size(), 2);
    
    // Test decim=24 (24 pairs = 1 complete block)
    std::vector<std::complex<float>> out24;
    downsample_iq(base_data, out24, 24);
    ASSERT_EQ(out24.size(), 1);
    
    // Verify: sum of all 3 blocks of decim=8 should equal the single block of decim=24
    std::complex<float> sum8(0, 0);
    for (const auto& val : out8) {
        sum8 += val;
    }
    EXPECT_TRUE(approx_equal(sum8, out24[0]));
    
    // Verify: sum of all 2 blocks of decim=12 should equal the single block of decim=24
    std::complex<float> sum12(0, 0);
    for (const auto& val : out12) {
        sum12 += val;
    }
    EXPECT_TRUE(approx_equal(sum12, out24[0]));
}

TEST(DownsampleIQTest, AlternatingSignPattern) {
    // Test with alternating positive/negative pattern
    std::vector<int16_t> in;
    for (int i = 0; i < 16; i++) {
        in.push_back((i % 2 == 0) ? 100 : -100);   // I: alternating
        in.push_back((i % 2 == 0) ? -50 : 50);     // Q: alternating opposite
    }
    
    std::vector<std::complex<float>> out;
    downsample_iq(in, out, 16);
    
    ASSERT_EQ(out.size(), 1);
    // 8 positive + 8 negative = 0 for both I and Q
    EXPECT_TRUE(approx_equal(out[0], std::complex<float>(0.0f, 0.0f)));
}

TEST(DownsampleIQTest, LargePositiveValues) {
    // Test with large positive values (but less than max)
    std::vector<int16_t> in(16, 20000);  // 8 IQ pairs
    std::vector<std::complex<float>> out;
    
    downsample_iq(in, out, 8);
    
    ASSERT_EQ(out.size(), 1);
    float expected = 20000.0f * 8;
    EXPECT_TRUE(approx_equal(out[0].real(), expected, 1.0f))
        << "Expected I: " << expected << ", Got: " << out[0].real();
    EXPECT_TRUE(approx_equal(out[0].imag(), expected, 1.0f))
        << "Expected Q: " << expected << ", Got: " << out[0].imag();
}

TEST(DownsampleIQTest, LargeNegativeValues) {
    // Test with large negative values
    std::vector<int16_t> in(16, -20000);  // 8 IQ pairs
    std::vector<std::complex<float>> out;
    
    downsample_iq(in, out, 8);
    
    ASSERT_EQ(out.size(), 1);
    float expected = -20000.0f * 8;
    EXPECT_TRUE(approx_equal(out[0].real(), expected, 1.0f))
        << "Expected I: " << expected << ", Got: " << out[0].real();
    EXPECT_TRUE(approx_equal(out[0].imag(), expected, 1.0f))
        << "Expected Q: " << expected << ", Got: " << out[0].imag();
}

TEST(DownsampleIQTest, MixedExtremeValues) {
    // Test mixing max and min values
    std::vector<int16_t> in;
    for (int i = 0; i < 4; i++) {
        in.push_back(32767);   // I: max
        in.push_back(-32768);  // Q: min
    }
    for (int i = 0; i < 4; i++) {
        in.push_back(-32768);  // I: min
        in.push_back(32767);   // Q: max
    }
    
    std::vector<std::complex<float>> out;
    downsample_iq(in, out, 8);
    
    ASSERT_EQ(out.size(), 1);
    float expected_i = 32767.0f * 4 + (-32768.0f) * 4;  // -4
    float expected_q = (-32768.0f) * 4 + 32767.0f * 4;  // -4
    EXPECT_TRUE(approx_equal(out[0].real(), expected_i, 1.0f))
        << "Expected I: " << expected_i << ", Got: " << out[0].real();
    EXPECT_TRUE(approx_equal(out[0].imag(), expected_q, 1.0f))
        << "Expected Q: " << expected_q << ", Got: " << out[0].imag();
}

TEST(DownsampleIQTest, Decimation10Multiple) {
    // Create 20 IQ pairs (40 samples)
    std::vector<int16_t> in;
    for (int i = 0; i < 20; i++) {
        in.push_back(i);      // I
        in.push_back(i + 100); // Q
    }
    
    std::vector<std::complex<float>> out;
    downsample_iq(in, out, 10);
    
    ASSERT_EQ(out.size(), 2);
    
    // First block: I = sum(0..9), Q = sum(100..109)
    float expected_i1 = 0 + 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9;
    float expected_q1 = 100 + 101 + 102 + 103 + 104 + 105 + 106 + 107 + 108 + 109;
    EXPECT_TRUE(approx_equal(out[0], std::complex<float>(expected_i1, expected_q1)));
    
    // Second block: I = sum(10..19), Q = sum(110..119)
    float expected_i2 = 10 + 11 + 12 + 13 + 14 + 15 + 16 + 17 + 18 + 19;
    float expected_q2 = 110 + 111 + 112 + 113 + 114 + 115 + 116 + 117 + 118 + 119;
    EXPECT_TRUE(approx_equal(out[1], std::complex<float>(expected_i2, expected_q2)));
}

TEST(DownsampleIQTest, ClearsOutput) {
    std::vector<int16_t> in = {1, 2, 3, 4};
    std::vector<std::complex<float>> out = {std::complex<float>(99, 99)};
    
    downsample_iq(in, out, 2);
    
    ASSERT_EQ(out.size(), 1);
    EXPECT_TRUE(approx_equal(out[0], std::complex<float>(4, 6)));
}

TEST(DemodulateFMTest, EmptyInput) {
    std::vector<std::complex<float>> in;
    std::vector<float> out;
    DemodState state{};
    
    demodulate_fm(in, out, state);
    
    EXPECT_TRUE(out.empty());
}

TEST(DemodulateFMTest, SingleSample) {
    std::vector<std::complex<float>> in = {{1.0f, 0.0f}};
    std::vector<float> out;
    DemodState state{std::complex<float>(1.0f, 0.0f)};
    
    demodulate_fm(in, out, state);
    
    ASSERT_EQ(out.size(), 1);
    EXPECT_TRUE(approx_equal(out[0], 0.0f));  // No phase change
}

TEST(DemodulateFMTest, PhaseShift) {
    // Test with 90 degree phase shift
    std::vector<std::complex<float>> in = {{0.0f, 1.0f}};
    std::vector<float> out;
    DemodState state{std::complex<float>(1.0f, 0.0f)};  // Previous sample at 0 deg
    
    demodulate_fm(in, out, state);
    
    ASSERT_EQ(out.size(), 1);
    EXPECT_TRUE(approx_equal(out[0], std::numbers::pi_v<float> / 2.0f, 1e-4f));
}

TEST(DemodulateFMTest, StatePreservation) {
    std::vector<std::complex<float>> in = {{1.0f, 1.0f}, {-1.0f, 1.0f}};
    std::vector<float> out;
    DemodState state{std::complex<float>(1.0f, 0.0f)};
    
    demodulate_fm(in, out, state);
    
    ASSERT_EQ(out.size(), 2);
    // State should be updated to last sample
    EXPECT_TRUE(approx_equal(state.prev_iq, std::complex<float>(-1.0f, 1.0f)));
}

TEST(DemodulateFMTest, ConsecutiveCalls) {
    DemodState state{std::complex<float>(1.0f, 0.0f)};
    std::vector<float> out1, out2;
    
    std::vector<std::complex<float>> in1 = {{1.0f, 0.0f}};
    demodulate_fm(in1, out1, state);
    
    std::vector<std::complex<float>> in2 = {{0.0f, 1.0f}};
    demodulate_fm(in2, out2, state);
    
    ASSERT_EQ(out2.size(), 1);
    // Phase difference between (1,0) and (0,1) is 90 degrees
    EXPECT_TRUE(approx_equal(out2[0], std::numbers::pi_v<float> / 2.0f, 1e-4f));
}

TEST(DemodulateAMTest, EmptyInput) {
    std::vector<std::complex<float>> in;
    std::vector<float> out;
    
    demodulate_am(in, out);
    
    EXPECT_TRUE(out.empty());
}

TEST(DemodulateAMTest, SingleSample) {
    std::vector<std::complex<float>> in = {{3.0f, 4.0f}};  // magnitude = 5
    std::vector<float> out;
    
    demodulate_am(in, out);
    
    ASSERT_EQ(out.size(), 1);
    EXPECT_TRUE(approx_equal(out[0], 5.0f));
}

TEST(DemodulateAMTest, MultipleSamples) {
    std::vector<std::complex<float>> in = {
        {3.0f, 4.0f},   // mag = 5
        {0.0f, 1.0f},   // mag = 1
        {1.0f, 0.0f}    // mag = 1
    };
    std::vector<float> out;
    
    demodulate_am(in, out);
    
    ASSERT_EQ(out.size(), 3);
    EXPECT_TRUE(approx_equal(out[0], 5.0f));
    EXPECT_TRUE(approx_equal(out[1], 1.0f));
    EXPECT_TRUE(approx_equal(out[2], 1.0f));
}

TEST(DemodulateAMTest, ZeroMagnitude) {
    std::vector<std::complex<float>> in = {{0.0f, 0.0f}};
    std::vector<float> out;
    
    demodulate_am(in, out);
    
    ASSERT_EQ(out.size(), 1);
    EXPECT_TRUE(approx_equal(out[0], 0.0f));
}

TEST(DemodulateAMTest, ManyValues) {
    std::vector<std::complex<float>> in;
    for (int i = 0; i < 10; i++) {
        in.push_back({static_cast<float>(i), static_cast<float>(i)});
    }
    std::vector<float> out;
    
    demodulate_am(in, out);
    
    ASSERT_EQ(out.size(), 10);
    for (int i = 0; i < 10; i++) {
        float expected = std::sqrt(2.0f * i * i);
        EXPECT_TRUE(approx_equal(out[i], expected)) 
            << "Index " << i << ": expected " << expected << ", got " << out[i];
    }
}

TEST(DemodulateAMTest, ClearsOutput) {
    std::vector<std::complex<float>> in = {{1.0f, 0.0f}};
    std::vector<float> out = {99.0f, 88.0f};
    
    demodulate_am(in, out);
    
    ASSERT_EQ(out.size(), 1);
    EXPECT_TRUE(approx_equal(out[0], 1.0f));
}

TEST(DownsampleAudioTest, EmptyInput) {
    std::vector<float> in;
    std::vector<float> out;
    AudioDecimState state{};
    
    downsample_audio(in, out, 2, state, 1.0f);
    
    EXPECT_TRUE(out.empty());
}

TEST(DownsampleAudioTest, InsufficientData) {
    std::vector<float> in = {1.0f};
    std::vector<float> out;
    AudioDecimState state{};
    
    downsample_audio(in, out, 2, state, 1.0f);
    
    EXPECT_TRUE(out.empty());
    EXPECT_EQ(state.counter, 1);
    EXPECT_TRUE(approx_equal(state.accumulator, 1.0f));
}

TEST(DownsampleAudioTest, SimpleDecimation) {
    std::vector<float> in = {1.0f, 2.0f, 3.0f, 4.0f};
    std::vector<float> out;
    AudioDecimState state{};
    
    downsample_audio(in, out, 2, state, 1.0f);
    
    ASSERT_EQ(out.size(), 2);
    EXPECT_TRUE(approx_equal(out[0], (1.0f + 2.0f) / 2.0f));  // 1.5
    EXPECT_TRUE(approx_equal(out[1], (3.0f + 4.0f) / 2.0f));  // 3.5
}

TEST(DownsampleAudioTest, WithGain) {
    std::vector<float> in = {1.0f, 2.0f, 3.0f, 4.0f};
    std::vector<float> out;
    AudioDecimState state{};
    
    downsample_audio(in, out, 2, state, 2.0f);  // 2x gain
    
    ASSERT_EQ(out.size(), 2);
    EXPECT_TRUE(approx_equal(out[0], (1.0f + 2.0f)));  // 3.0 (sum * gain/decim)
    EXPECT_TRUE(approx_equal(out[1], (3.0f + 4.0f)));  // 7.0
}

TEST(DownsampleAudioTest, StatePreservation) {
    std::vector<float> in1 = {1.0f, 2.0f, 3.0f};
    std::vector<float> out1;
    AudioDecimState state{};
    
    downsample_audio(in1, out1, 2, state, 1.0f);
    
    ASSERT_EQ(out1.size(), 1);
    EXPECT_EQ(state.counter, 1);
    EXPECT_TRUE(approx_equal(state.accumulator, 3.0f));
    
    // Continue with more data
    std::vector<float> in2 = {4.0f, 5.0f, 6.0f};
    std::vector<float> out2;
    
    downsample_audio(in2, out2, 2, state, 1.0f);
    
    ASSERT_EQ(out2.size(), 2);
    EXPECT_TRUE(approx_equal(out2[0], (3.0f + 4.0f) / 2.0f));  // 3.5 (leftover + new)
    EXPECT_TRUE(approx_equal(out2[1], (5.0f + 6.0f) / 2.0f));  // 5.5
}

TEST(DownsampleAudioTest, ClearsOutput) {
    std::vector<float> in = {1.0f, 2.0f};
    std::vector<float> out = {99.0f};
    AudioDecimState state{};
    
    downsample_audio(in, out, 2, state, 1.0f);
    
    ASSERT_EQ(out.size(), 1);
    EXPECT_TRUE(approx_equal(out[0], 1.5f));
}

TEST(DownsampleAudioTest, Decim10) {
    std::vector<float> in(10, 1.0f);
    std::vector<float> out;
    AudioDecimState state{};
    
    downsample_audio(in, out, 10, state, 1.0f);
    
    ASSERT_EQ(out.size(), 1);
    EXPECT_TRUE(approx_equal(out[0], 1.0f));  // 10 * 1.0 / 10
}


// Integration Tests


TEST(IntegrationTest, IQToAudioPipeline) {
    // Simulate a simple IQ -> FM demod -> audio downsample pipeline
    
    // Generate simple IQ data
    std::vector<int16_t> iq_data;
    for (int i = 0; i < 40; i++) {
        iq_data.push_back(100 * std::cos(i * 0.1));
        iq_data.push_back(100 * std::sin(i * 0.1));
    }
    
    // Downsample IQ
    std::vector<std::complex<float>> downsampled_iq;
    downsample_iq(iq_data, downsampled_iq, 2);
    ASSERT_FALSE(downsampled_iq.empty());
    
    // FM Demodulation
    std::vector<float> fm_out;
    DemodState fm_state{};
    demodulate_fm(downsampled_iq, fm_out, fm_state);
    ASSERT_EQ(fm_out.size(), downsampled_iq.size());
    
    // Audio downsampling
    std::vector<float> audio_out;
    AudioDecimState audio_state{};
    downsample_audio(fm_out, audio_out, 2, audio_state, 1.0f);
    EXPECT_FALSE(audio_out.empty());
}

TEST(IntegrationTest, IQToAMPipeline) {
    // Simulate IQ -> AM demod -> audio downsample pipeline
    
    std::vector<int16_t> iq_data = {100, 0, 100, 0, 100, 0, 100, 0};
    
    std::vector<std::complex<float>> downsampled_iq;
    downsample_iq(iq_data, downsampled_iq, 2);
    
    std::vector<float> am_out;
    demodulate_am(downsampled_iq, am_out);
    ASSERT_EQ(am_out.size(), downsampled_iq.size());
    
    std::vector<float> audio_out;
    AudioDecimState audio_state{};
    downsample_audio(am_out, audio_out, 2, audio_state, 1.0f);
    EXPECT_FALSE(audio_out.empty());
}
