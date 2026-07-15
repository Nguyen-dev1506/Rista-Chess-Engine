CXX = g++
CXXFLAGS = -O3 -Wall -Wextra -std=c++20 -march=native -flto -pthread -Isrc

SRCS_CPP = $(wildcard src/*.cpp)
SRCS_FATHOM = src/fathom/tbprobe.cpp
OBJS_CPP = $(patsubst src/%.cpp,build/%.o,$(SRCS_CPP))
OBJS_FATHOM = $(patsubst src/fathom/%.cpp,build/fathom/%.o,$(SRCS_FATHOM))
OBJS = $(OBJS_CPP) $(OBJS_FATHOM)
TARGET = rista

all: build $(TARGET)

build:
	mkdir -p build build/fathom

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

build/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c "$<" -o "$@"

build/fathom/%.o: src/fathom/%.cpp
	$(CXX) $(CXXFLAGS) -c "$<" -o "$@"

clean:
	rm -rf build $(TARGET)
