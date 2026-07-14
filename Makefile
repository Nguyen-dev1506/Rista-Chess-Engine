CXX = g++
CXXFLAGS = -O3 -Wall -Wextra -std=c++20 -march=native -flto -pthread -Isrc

SRCS = $(wildcard src/*.cpp)
OBJS = $(patsubst src/%.cpp,build/%.o,$(wildcard src/*.cpp))
TARGET = rista

all: build $(TARGET)

build:
	mkdir -p build

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

build/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c "$<" -o "$@"

clean:
	rm -rf build $(TARGET)
