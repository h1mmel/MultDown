OBJ_DIR := obj
SRC_DIR := downloader
BIN_DIR := bin
INCLUDE_DIR := include

CXX := g++ -fsanitize=address -fsanitize=leak
CXXFLAGS := -std=c++11 -g -Wall -I$(INCLUDE_DIR) -I.
LDLIBS := -lpthread -lstdc++ -lcurl

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))
TARGET := $(BIN_DIR)/mdown

.DEFAULT_GOAL := $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) main.cpp -o $@ $^ $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $< $(LDLIBS)

.PHONY: clean
clean:
	rm -rf $(OBJS)
	rm -rf $(TARGET)
	rm -rf bin/ obj/