CXX = g++
CXXFLAGS_STD = -std=c++17
CXXFLAGS_RELEASE = $(CXXFLAGS_STD) -O3 -march=native -mtune=native
CXXFLAGS_DEBUG = $(CXXFLAGS_STD) -g -Wall -DDEBUG_MODE=true
TARGET = vm
SRCS = vm.cpp
OBJS = $(SRCS:.cpp=.o)

all: release

release: CXXFLAGS = $(CXXFLAGS_RELEASE)
release: $(TARGET)
	@echo "--- Built $(TARGET) (Release) ---"

debug: CXXFLAGS = $(CXXFLAGS_DEBUG)
debug: $(TARGET)
	@echo "--- Built $(TARGET) (Debug) ---"

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "--- Cleaning project ---"
	$(RM) $(OBJS) $(TARGET)

.PHONY: all clean release debug
