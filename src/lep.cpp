#include <termios.h>
#include "../include/LepEditor.h"

using namespace std;

struct termios defSet;

void dRawMode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &defSet);
}

void eRawMode() {
  tcgetattr(STDIN_FILENO, &defSet);
  atexit(dRawMode);

  struct termios raw = defSet;
  raw.c_lflag &= ~(ECHO | ICANON);
  raw.c_cc[VTIME] = 1; //Timeout time in 1/10 of seconds
  raw.c_cc[VMIN] = 0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void run(LepEditor* lep) {
  while(1) {
    switch (lep->currentState) {
      case State::INPUT:
        lep->modeInput();
        break;

      case State::COMMAND:
        lep->modeCommand();
        break;

      case State::VLINE:
      case State::VISUAL:
        lep->modeVisual();
        break;
  
      default:
        lep->modeNormal();      
        break;
    }
  }
}

int main(int argc, char** argv) {
	system("clear");
	eRawMode();

  string fileName;
  if (argc >= 2) fileName = argv[1];
  else {
    fileName = "";
  }

  LepEditor lep;
  lep.start(fileName);

  run(&lep);

	return 0;
}

