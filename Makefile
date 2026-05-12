CXX = g++

# =========================
# Build Flags
# =========================

DEBUG_FLAGS = -std=c++17 -Wall -Wextra -g -O0 -Iinclude
RELEASE_FLAGS = -std=c++17 -Wall -Wextra -O3 -Iinclude

CXXFLAGS = $(DEBUG_FLAGS)

# Default (Unix/macOS)
LDFLAGS = -pthread

# Detect Windows
ifeq ($(OS),Windows_NT)
    LDFLAGS += -lws2_32 -lmswsock
endif

# =========================
# Source Files
# =========================

SRC = $(filter-out src/main.cpp, $(wildcard src/*.cpp))
MAIN_SRC = src/main.cpp

OBJ = $(SRC:.cpp=.o)
BENCH_OBJ = $(SRC:.cpp=.bench.o)

TEST_SRC = $(wildcard tests/*.cpp)
BENCH_SRC = $(wildcard benchmarks/*.cpp)

# =========================
# Output Executables
# =========================

OUT = search_engine
TEST_OUT = runTests
BENCH_OUT = runBenchmarks

# =========================
# Main Targets
# =========================

all: $(OUT) $(TEST_OUT) $(BENCH_OUT)

# =========================
# Main Search Engine
# =========================

$(OUT): $(OBJ) $(MAIN_SRC)
	$(CXX) $(CXXFLAGS) $(OBJ) $(MAIN_SRC) \
	-o $(OUT) $(LDFLAGS)

# =========================
# Unit Tests
# =========================

$(TEST_OUT): $(TEST_SRC) $(OBJ)
	$(CXX) $(CXXFLAGS) $(TEST_SRC) $(OBJ) \
	-o $(TEST_OUT) \
	-lgtest -lgtest_main $(LDFLAGS)

# =========================
# Benchmarks
# =========================

$(BENCH_OUT): $(BENCH_SRC) $(BENCH_OBJ)
	$(CXX) $(RELEASE_FLAGS) $(BENCH_SRC) $(BENCH_OBJ) \
	-o $(BENCH_OUT) \
	-lbenchmark $(LDFLAGS)

# =========================
# Debug Object Compilation
# =========================

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# =========================
# Release Benchmark Objects
# =========================

%.bench.o: %.cpp
	$(CXX) $(RELEASE_FLAGS) -DBENCHMARK_MODE \
	-c $< -o $@

# =========================
# Convenience Commands
# =========================

run: $(OUT)
	./$(OUT)

test: $(TEST_OUT)
	./$(TEST_OUT)

bench: $(BENCH_OUT)
	./$(BENCH_OUT)

debug: $(OUT)
	gdb ./$(OUT)

# =========================
# Cleanup
# =========================

clean:
	rm -f src/*.o
	rm -f src/*.bench.o
	rm -f $(OUT)
	rm -f $(TEST_OUT)
	rm -f $(BENCH_OUT)