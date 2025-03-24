CXX = g++
BUILD_DIR = builds
SRC_DIR = main
SRC = $(wildcard $(SRC_DIR)/*.cpp)
TARGETS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%, $(SRC))

all: $(BUILD_DIR) $(TARGETS)

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

$(BUILD_DIR)/%: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	del /Q $(BUILD_DIR)\*.exe
	rmdir /S /Q $(BUILD_DIR)
