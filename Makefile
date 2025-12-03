CXX       := g++
CXXFLAGS  := -std=c++20 -O3 -Wall -Wextra -Wpedantic -march=native
LDFLAGS   := -liio

# Enable NEON if the compiler target supports it
# (this will auto-detect ARM NEON on ARM CPUs)
ARCH := $(shell $(CXX) -dumpmachine)

# ARM64 (AArch64)
ifeq ($(findstring aarch64,$(ARCH)),aarch64)
    $(info Detected AArch64 → enabling NEON (armv8-a+simd))
    CXXFLAGS += -march=armv8-a+simd -D__ARM_NEON__
endif

# ARMv7
ifeq ($(findstring armv7,$(ARCH)),armv7)
    $(info Detected ARMv7 → enabling NEON (-mfpu=neon))
    CXXFLAGS += -mfpu=neon -D__ARM_NEON__
endif

# Source files
SRCS := \
    main.cpp \
    dsp.cpp \
    udp_sender.cpp \
    plutosdr.cpp

# Object files (inside build/)
OBJS := $(addprefix build/,$(SRCS:.cpp=.o))
DEPS := $(OBJS:.o=.d)

TARGET := fm_radio

# ----------------------------
# Build rules
# ----------------------------

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

# Build directory
build/:
	mkdir -p build

# Object compilation with dependency generation
build/%.o: %.cpp | build/
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# Include auto-generated dependency files
-include $(DEPS)

clean:
	rm -rf build $(TARGET)
