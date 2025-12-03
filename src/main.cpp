/**
 * @file main.cpp
 * @brief PlutoSDR FM receiver entry point with CLI parsing.
 */

#include <charconv>
#include <cmath>
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

#include "plutosdr.hpp"

/// Print available command-line options.
static void print_usage(std::string_view prog)
{
    std::cerr <<
        "Usage:\n"
        "  " << prog << " -f <freq_mhz> [-g <gain_db>] [-a <ip>] [-p <port>]\n";
}

/// Print compile-time SIMD configuration.
static void print_simd_info()
{
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    std::cerr << "NEON SIMD enabled\n";
#else
    std::cerr << "NEON SIMD disabled\n";
#endif
}

static bool parse_double(std::string_view sv, double& out)
{
    try { out = std::stod(std::string(sv)); return true; }
    catch (...) { return false; }
}

static bool parse_freq_mhz(std::string_view sv, long long& hz)
{
    double mhz;
    if (!parse_double(sv, mhz)) return false;
    hz = static_cast<long long>(std::llround(mhz * 1'000'000.0));
    return true;
}

static bool parse_port(std::string_view sv, int& out)
{
    long long tmp{};
    auto r = std::from_chars(sv.data(), sv.data() + sv.size(), tmp);
    if (r.ec != std::errc{} || tmp < 1 || tmp > 65535) return false;
    out = static_cast<int>(tmp);
    return true;
}

int main(int argc, char* argv[])
{
    std::ios::sync_with_stdio(false);

    long long freq_hz = 0;
    double gain_db = 0.0;
    std::optional<std::string> udp_ip;
    std::optional<int> udp_port;

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    try {
        for (int i = 1; i < argc; ++i) {
            std::string_view arg = argv[i];

            auto next = [&](std::string_view opt) {
                if (i + 1 >= argc) throw std::runtime_error("Missing value for " + std::string(opt));
                return std::string_view(argv[++i]);
            };

            if (arg == "-h" || arg == "--help") {
                print_usage(argv[0]);
                return 0;
            }
            if (arg == "-f" || arg == "--frequency") {
                if (!parse_freq_mhz(next(arg), freq_hz))
                    throw std::runtime_error("Invalid frequency");
            }
            else if (arg == "-g" || arg == "--gain") {
                if (!parse_double(next(arg), gain_db))
                    throw std::runtime_error("Invalid gain");
            }
            else if (arg == "-a" || arg == "--address") {
                udp_ip = std::string(next(arg));
            }
            else if (arg == "-p" || arg == "--port") {
                int p;
                if (!parse_port(next(arg), p))
                    throw std::runtime_error("Invalid port");
                udp_port = p;
            }
            else {
                throw std::runtime_error("Unknown argument");
            }
        }

        if (!freq_hz) {
            print_usage(argv[0]);
            return 1;
        }

        print_simd_info();

        PlutoSDR radio(freq_hz, gain_db, udp_ip, udp_port);
        radio.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
