SRC := ./src
INC := -I ./include
FLAGS := -c $(INC)
CC := g++

OBJS := lep.o ModeState.o

lep: $(OBJS)
	$(CC) $^ -o $@

%.o: $(SRC)/%.cpp
	$(CC) $(FLAGS) $<

debug:
	$(CC) $(INC) $(SRC)/*.cpp -pthread -g -o lepDebug
	gdb -tui lepDebug

clean:
	rm -rf *.o lepDebug

run:
	./lep
