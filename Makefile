CXX       := g++
CXXFLAGS  := -std=c++20 -O3 -Wall -Wextra -Wpedantic
LDFLAGS   := -liio

TEST_CXXFLAGS := -std=c++20 -g -O0 -Wall -Wextra -Wpedantic
TEST_LDFLAGS  := -lgtest -lgtest_main -lpthread -liio

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
TESTBUILDDIR := build/test_objs

SRCS := $(wildcard $(SRCDIR)/*.cpp)
MAIN_SRC := $(SRCDIR)/main.cpp
LIB_SRCS := $(filter-out $(MAIN_SRC),$(SRCS))

TEST_SRC := $(TESTDIR)/dsp_test.cpp
BENCH_SRC := $(TESTDIR)/dsp_benchmark.cpp

OBJS := $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(SRCS))
LIB_OBJS := $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(LIB_SRCS))
TEST_OBJ := $(TESTBUILDDIR)/dsp_test.o
BENCH_OBJ := $(TESTBUILDDIR)/dsp_benchmark.o

DEPS := $(OBJS:.o=.d)
TEST_DEP := $(TEST_OBJ:.o=.d)
BENCH_DEP := $(BENCH_OBJ:.o=.d)

TARGET := $(BUILDDIR)/fm_radio
TEST_TARGET := $(BUILDDIR)/tests
BENCH_TARGET := $(BUILDDIR)/benchmark_runner

.PHONY: all clean test benchmark check

all: $(TARGET)

$(TARGET): $(OBJS) | $(BUILDDIR)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

$(TEST_TARGET): $(LIB_OBJS) $(TEST_OBJ) | $(BUILDDIR)
	$(CXX) $(LIB_OBJS) $(TEST_OBJ) $(TEST_LDFLAGS) -o $@

$(BENCH_TARGET): $(LIB_OBJS) $(BENCH_OBJ) | $(BUILDDIR)
	$(CXX) $(LIB_OBJS) $(BENCH_OBJ) $(BENCH_LDFLAGS) -o $@

test: $(TEST_TARGET)
	@echo "=== Running Tests ==="
	./$(TEST_TARGET)

benchmark: $(BENCH_TARGET)
	@echo "=== Running Benchmarks ==="
	./$(BENCH_TARGET)

check: test benchmark

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(TESTBUILDDIR):
	mkdir -p $(TESTBUILDDIR)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(TEST_OBJ): $(TEST_SRC) | $(TESTBUILDDIR)
	$(CXX) $(TEST_CXXFLAGS) -I$(SRCDIR) -MMD -MP -c $< -o $@

$(BENCH_OBJ): $(BENCH_SRC) | $(TESTBUILDDIR)
	$(CXX) $(BENCH_CXXFLAGS) -I$(SRCDIR) -MMD -MP -c $< -o $@

-include $(DEPS)
-include $(TEST_DEP)
-include $(BENCH_DEP)

clean:
	rm -rf $(BUILDDIR)

help:
	@echo "Available targets:"
	@echo "  all          - Build main application (in $(BUILDDIR)/)"
	@echo "  test         - Build and run unit tests"
	@echo "  benchmark    - Build and run benchmarks"
	@echo "  check        - Run both tests and benchmarks"
	@echo "  clean        - Remove build directory"
	@echo "  help         - Show this help message"