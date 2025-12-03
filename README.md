# ADALM-PLUT SDR FM Radio Receiver

A lightweight, fast, C++20 FM radio receiver for the ADALM-PLUTO (PlutoSDR).

Features:

- DSP chain (IQ decimation -> FM demod -> audio output)
- Pure C++20 DSP
- Optional UDP streaming
- Output via stdout (F32LE, 48 kHz)
- ARM NEON acceleration (auto-detected at build time)

## Build

### Dependencies:

- `g++` with C++20
- [libiio](https://github.com/analogdevicesinc/libiio) installed

### Build:
```make
make
```

### Clean:
```make
make clean
```

### Usage

```bash
./fm_radio -f <freq_mhz> [-g <gain_db>] [-a <ip>] [-p <port>]
```

### Options

| Option              | Description                        |
| ------------------- | ---------------------------------- |
| `-f`, `--frequency` | FM frequency in **MHz** (required) |
| `-g`, `--gain`      | RF gain in dB (default: 0 dB)      |
| `-a`, `--address`   | Optional UDP IPv4 address          |
| `-p`, `--port`      | Optional UDP port                  |
| `-h`, `--help`      | Show help                          |


If no UDP address/port is given, float audio is written to stdout as:

```text
audio/x-raw, format=F32LE, rate=48000, channels=1
```

Perfect for use with GStreamer or ffmpeg.

## Examples

### Play audio via GStreamer

```bash
./fm_radio -f 98.4 | \
  gst-launch-1.0 fdsrc ! \
  audio/x-raw,format=F32LE,rate=48000,channels=1 ! \
  autoaudiosink
```

### Multicast stream

```bash
./fm_radio -f 98.4 -a 224.1.1.1 -p 5000
```

### Save to file

```bash
./fm_radio -f 98.4 > audio.raw
```

## NEON SIMD Support

If compiled on ARM with NEON, the program prints:

```bash
NEON SIMD: enabled
```

Otherwise:

```bash
NEON SIMD: disabled
```

### Project Structure

**dsp.hpp / dsp.cpp**               – DSP functions (IQ downsampling, FM demod, audio)  
**udp_sender.hpp / udp_sender.cpp** – UDP transmission  
**plutosdr.hpp / plutosdr.cpp**     – PlutoSDR hardware + DSP integration  
**main.cpp**                        – Command-line interface and entry point  
**Makefile**                        – Make script  
