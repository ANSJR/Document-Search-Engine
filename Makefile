CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -O0 -Iinclude

SRC = $(wildcard src/*.cpp)
OBJ = $(SRC:.cpp=.o)

OUT = search_engine

all: $(OUT)

$(OUT): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(OUT)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: all
	./$(OUT)

debug: all
	gdb ./$(OUT)

clean:
	rm -f src/*.o $(OUT)