SRC := ./src
BIN := ./bin
BUILD := ./bin/build
OBJS := ./objs
INC := -I ./include
FLAGS := -c $(INC)
LINK := 
CC := g++
MAIN := lim
PLATFORM := $(shell uname)

OBJ := LimEditor ModeState Clipboard Clip Config Filetree
OBJECT_FILES := $(addprefix $(OBJS)/,$(addsuffix .o,$(OBJ)))

$(MAIN): $(OBJECT_FILES) | $(BIN)
	$(CC) $^ $(SRC)/$@.cpp -o $(BIN)/$@ $(LINK)

$(OBJS)/%.o: $(SRC)/%.cpp | $(OBJS)
	$(CC) $(FLAGS) $< -o $@

$(OBJS):
	mkdir $(OBJS)

$(BIN):
	mkdir $(BIN)

$(BUILD): $(BIN)
	mkdir $(BUILD)

build: $(OBJECT_FILES) $(OBJS)/$(MAIN).o | $(BUILD)
	$(CC) -static $^ -o $(BUILD)/$(MAIN) $(LINK)

debug:
	$(CC) $(INC) $(SRC)/*.cpp -pthread -g -o $(BIN)/limDebug
	gdb -tui $(BIN)/limDebug

clean:
	rm -rf $(OBJS)/*.o $(BIN)/*

run:
	$(BIN)/lim
