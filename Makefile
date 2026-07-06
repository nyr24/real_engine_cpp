SRC_DIR=src
DEBUG_BUILD_DIR=build/debug
RELEASE_BUILD_DIR=build/release
INCLUDE_DIR=include
SOURCES=$(wildcard $(SRC_DIR)/*.cpp)
DEBUG_OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(DEBUG_BUILD_DIR)/%.o, $(SOURCES))
RELEASE_OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(RELEASE_BUILD_DIR)/%.o, $(SOURCES))
EXE=app
COMPILER=clang++
CPP_STANDART=-std=c++20
FLAGS=-march=native -Wall
DEBUG_FLAGS=$(CPP_STANDART) -g -O0 $(FLAGS)
RELEASE_FLAGS=$(CPP_STANDART) -O3 -flto $(FLAGS)

.PHONY: debug release prepare clean run run_r rebuild rerun rebuild_r rerun_r

debug: prepare $(DEBUG_BUILD_DIR)/$(EXE)

$(DEBUG_BUILD_DIR)/$(EXE): $(DEBUG_OBJECTS)
	$(COMPILER) $(DEBUG_FLAGS) -o $@ $^
	@echo "Debug build completed!"

$(DEBUG_BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(COMPILER) $(DEBUG_FLAGS) -o $@ -c $<

release: prepare $(RELEASE_BUILD_DIR)/$(EXE)

$(RELEASE_BUILD_DIR)/$(EXE): $(RELEASE_OBJECTS)
	$(COMPILER) $(RELEASE_FLAGS) -o $@ $^
	@echo "Release build completed!"

$(RELEASE_BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(COMPILER) $(RELEASE_FLAGS) -o $@ -c $<

run:
	@echo "Running in debug mode"
	$(DEBUG_BUILD_DIR)/$(EXE)

run_r:
	@echo "Running in release mode"
	$(RELEASE_BUILD_DIR)/$(EXE)

rebuild: clean prepare debug

rebuild_r: clean prepare release

rerun: rebuild run

rerun_r: rebuild run_r
	
prepare:
	@mkdir -p $(DEBUG_BUILD_DIR)
	@mkdir -p $(RELEASE_BUILD_DIR)

clean:
	@rm -f $(DEBUG_BUILD_DIR)/$(EXE) $(DEBUG_BUILD_DIR)/*.o
	@rm -f $(RELEASE_BUILD_DIR)/$(EXE) $(RELEASE_BUILD_DIR)/*.o
