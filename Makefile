CXX = g++

CXXFLAGS = -std=c++17 -Wall -Wextra -g -O0 -Iinclude

# Default (Unix/macOS)
LDFLAGS =

# Detect Windows
ifeq ($(OS),Windows_NT)
    LDFLAGS += -lws2_32 -lmswsock
endif

SRC = $(wildcard src/*.cpp)
OBJ = $(SRC:.cpp=.o)

OUT = search_engine

all: $(OUT)

$(OUT): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(OUT) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: all
	./$(OUT)

debug: all
	gdb ./$(OUT)

clean:
	rm -f src/*.o $(OUT)