.PHONY: all run clean compile

CXX = g++ -std=c++11
CXXFLAGS= -fdiagnostics-color=always -g -Wall -Iinclude -Wno-deprecated
LDFLAGS = library/libglfw.3.4.dylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -framework CoreFoundation -Llibrary -ldl -lpthread -lm -lfftw3

SRC_DIR     = src
BUILD_DIR   = binaries
TARGET      = $(OUT_FILE)

OUT_FILE = Application
VENDOR_FILES = $(SRC_DIR)/vendor/stb_image/stb_image.cpp $(wildcard $(SRC_DIR)/vendor/imgui/*.cpp) $(wildcard $(SRC_DIR)/tests/*.cpp) $(SRC_DIR)/vendor/miniaudio/miniaudio_implementation.cpp

C_SRCS      = $(wildcard $(SRC_DIR)/*.c)
CPP_SRCS    = $(wildcard $(SRC_DIR)/*.cpp)
CPP_SRCS    := $(filter-out $(SRC_DIR)/$(OUT_FILE).cpp, $(CPP_SRCS))

OBJS        = $(C_SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o) \
              $(CPP_SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

run: all
	clear
	leaks -quiet -list --atExit -- ./$(OUT_FILE)

all: compile

clean: $(OUT_FILE)
	rm $(OUT_FILE)

# Link the final executable
compile: $(OBJS)
	$(CXX) $(SRC_DIR)/$(OUT_FILE).cpp $(VENDOR_FILES) $(CXXFLAGS) $(LDFLAGS) -o $(TARGET) $^

# Compile C source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile C++ source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@