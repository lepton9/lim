SRC := ./src
INC := -I ./include
FLAGS := -c $(INC)
CC := g++

lep: lep.o ModeState.o
	${CC} $^ -o $@

lep.o: $(SRC)/lep.cpp
	$(CC) $(FLAGS) $<

ModeState.o: $(SRC)/ModeState.cpp
	$(CC) $(FLAGS) $<

debug:
	$(CC) $(INC) $(SRC)/*.cpp -pthread -g -o lepDebug
	gdb -tui lepDebug

clean:
	rm -rf *.o

run:
	./lep
