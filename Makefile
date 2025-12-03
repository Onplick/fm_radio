CXX       := g++
CXXFLAGS  := -std=c++20 -O3 -Wall -Wextra -Wpedantic
LDFLAGS   := -liio

ARCH := $(shell $(CXX) -dumpmachine)

ifeq ($(findstring aarch64,$(ARCH)),aarch64)
    $(info Detected ARMv8)
    CXXFLAGS += -march=armv8-a+simd -D__ARM_NEON__
endif

ifeq ($(findstring armv7,$(ARCH)),armv7)
    $(info Detected ARMv7)
    CXXFLAGS += -mfpu=neon -D__ARM_NEON__
endif

SRCDIR := src
BUILDDIR := build

SRCS := $(wildcard $(SRCDIR)/*.cpp)

OBJS := $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

TARGET := fm_radio

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(BUILDDIR) $(TARGET)