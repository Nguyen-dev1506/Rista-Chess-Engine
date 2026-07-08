CXX = g++
CXXFLAGS = -O3 -Wall -Wextra -std=c++11

SRCS = main.cpp board.cpp movegen.cpp search.cpp uci.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = rista

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
