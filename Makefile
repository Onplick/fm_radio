CXX       := g++
CXXFLAGS  := -std=c++20 -O3 -Wall -Wextra -Wpedantic
LDFLAGS   := -liio

# Test-specific flags
TEST_CXXFLAGS := -std=c++20 -g -O0 -Wall -Wextra -Wpedantic
TEST_LDFLAGS  := -lgtest -lgtest_main -lpthread -liio

# Benchmark-specific flags
BENCH_CXXFLAGS := -std=c++20 -O3 -Wall -Wextra -Wpedantic
BENCH_LDFLAGS  := -lbenchmark -lpthread -liio

ARCH := $(shell $(CXX) -dumpmachine)

ifeq ($(findstring aarch64,$(ARCH)),aarch64)
    $(info Detected ARMv8)
    CXXFLAGS += -march=armv8-a+simd -D__ARM_NEON__
    TEST_CXXFLAGS += -march=armv8-a+simd -D__ARM_NEON__
    BENCH_CXXFLAGS += -march=armv8-a+simd -D__ARM_NEON__
endif

ifeq ($(findstring armv7,$(ARCH)),armv7)
    $(info Detected ARMv7)
    CXXFLAGS += -mfpu=neon -D__ARM_NEON__
    TEST_CXXFLAGS += -mfpu=neon -D__ARM_NEON__
    BENCH_CXXFLAGS += -mfpu=neon -D__ARM_NEON__
endif

SRCDIR := src
TESTDIR := tests
BUILDDIR := build
TESTBUILDDIR := build/tests

# Main application sources (exclude main.cpp for tests)
SRCS := $(wildcard $(SRCDIR)/*.cpp)
MAIN_SRC := $(SRCDIR)/main.cpp
LIB_SRCS := $(filter-out $(MAIN_SRC),$(SRCS))

# Test and benchmark sources
TEST_SRC := $(TESTDIR)/dsp_test.cpp
BENCH_SRC := $(TESTDIR)/dsp_benchmark.cpp

# Object files
OBJS := $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(SRCS))
LIB_OBJS := $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(LIB_SRCS))
TEST_OBJ := $(TESTBUILDDIR)/dsp_test.o
BENCH_OBJ := $(TESTBUILDDIR)/dsp_benchmark.o

# Dependency files
DEPS := $(OBJS:.o=.d)
TEST_DEP := $(TEST_OBJ:.o=.d)
BENCH_DEP := $(BENCH_OBJ:.o=.d)

# Targets
TARGET := fm_radio
TEST_TARGET := test_runner
BENCH_TARGET := benchmark_runner

.PHONY: all clean test benchmark check

all: $(TARGET)

# Build main application
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

# Build test runner (Google Test)
$(TEST_TARGET): $(LIB_OBJS) $(TEST_OBJ)
	$(CXX) $(LIB_OBJS) $(TEST_OBJ) $(TEST_LDFLAGS) -o $@

# Build benchmark runner (Google Benchmark)
$(BENCH_TARGET): $(LIB_OBJS) $(BENCH_OBJ)
	$(CXX) $(LIB_OBJS) $(BENCH_OBJ) $(BENCH_LDFLAGS) -o $@

# Run tests
test: $(TEST_TARGET)
	@echo "=== Running Tests ==="
	./$(TEST_TARGET)

# Run benchmarks
benchmark: $(BENCH_TARGET)
	@echo "=== Running Benchmarks ==="
	./$(BENCH_TARGET)

# Run both tests and benchmarks
check: test benchmark

# Create build directories
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(TESTBUILDDIR):
	mkdir -p $(TESTBUILDDIR)

# Build main application objects
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# Build test object
$(TEST_OBJ): $(TEST_SRC) | $(TESTBUILDDIR)
	$(CXX) $(TEST_CXXFLAGS) -I$(SRCDIR) -MMD -MP -c $< -o $@

# Build benchmark object
$(BENCH_OBJ): $(BENCH_SRC) | $(TESTBUILDDIR)
	$(CXX) $(BENCH_CXXFLAGS) -I$(SRCDIR) -MMD -MP -c $< -o $@

# Include dependency files
-include $(DEPS)
-include $(TEST_DEP)
-include $(BENCH_DEP)

clean:
	rm -rf $(BUILDDIR) $(TARGET) $(TEST_TARGET) $(BENCH_TARGET)

# Help target
help:
	@echo "Available targets:"
	@echo "  all          - Build main application (default)"
	@echo "  test         - Build and run unit tests"
	@echo "  benchmark    - Build and run benchmarks"
	@echo "  check        - Run both tests and benchmarks"
	@echo "  clean        - Remove build artifacts"
	@echo "  help         - Show this help message"