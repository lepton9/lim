SRC := ./src
BIN := ./bin
OBJS := ./objs
INC := -I ./include
FLAGS := -c $(INC)
CC := g++

OBJ := ./objs/lim.o ./objs/LimEditor.o ./objs/ModeState.o ./objs/Clipboard.o ./objs/Clip.o ./objs/Config.o ./objs/Filetree.o

lim: $(OBJ)
	$(CC) $^ -o $(BIN)/$@

$(OBJS)/%.o: $(SRC)/%.cpp
	$(CC) $(FLAGS) $< -o $@

debug:
	$(CC) $(INC) $(SRC)/*.cpp -pthread -g -o $(BIN)/limDebug
	gdb -tui $(BIN)/limDebug

clean:
	rm -rf $(OBJS)/*.o $(BIN)/*

run:
	$(BIN)/lim
