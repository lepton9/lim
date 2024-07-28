#include <termios.h>
#include "../include/LimEditor.h"

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

void run(LimEditor* lim) {
  while(1) {
    switch (lim->currentState) {
      case State::INPUT:
        lim->modeInput();
        break;

      case State::COMMAND:
        lim->modeCommand();
        break;

      case State::VLINE:
      case State::VISUAL:
        lim->modeVisual();
        break;
  
      default:
        lim->modeNormal();      
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

  LimEditor lim;
  lim.start(fileName);

  run(&lim);

	return 0;
}

