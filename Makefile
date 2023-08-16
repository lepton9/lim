SRC := ./src
BIN := ./bin
OBJS := ./objs
INC := -I ./include
FLAGS := -c $(INC)
CC := g++

OBJ := ./objs/lep.o ./objs/ModeState.o ./objs/Clipboard.o ./objs/Clip.o ./objs/Config.o ./objs/Filetree.o

lep: $(OBJ)
	$(CC) $^ -o $(BIN)/$@

$(OBJS)/%.o: $(SRC)/%.cpp
	$(CC) $(FLAGS) $< -o $@

debug:
	$(CC) $(INC) $(SRC)/*.cpp -pthread -g -o $(BIN)/lepDebug
	gdb -tui $(BIN)/lepDebug

clean:
	rm -rf $(OBJS)/*.o $(BIN)/lepDebug

run:
	$(BIN)/lep
