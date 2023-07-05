
#include <cstdlib>
#include <ios>
#include <iostream>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <fstream>
#include <termios.h>
#include <vector>
#include "../include/ModeState.h"
#include "../include/stev.h"
//#include <conio.h>


using namespace std;

//Original terminal settings 
struct termios defSet;

class Lep : public ModeState {
  public:
    Lep() {}
    Lep(fstream* f) : file{f} {}
    ModeState state;

    void modeNormal() {
      char c;
      while(currentState == State::NORMAL) {

        read(STDIN_FILENO, &c, 1);
        if (iscntrl(c)) {
          switch (c) {
            case 27:
              cout << "esc" << endl;
              break;
            default:
              break;
          }

        } else {
          switch (c) {
            case 'i':
              cout << "i" << endl;
              handleEvent(Event::INPUT);
              break;
            case ':':
              cout << ":" << endl;
              handleEvent(Event::COMMAND);
              break;

            //Movement
            case 'h': //Left
              curLeft();
              break;
            case 'j': //Down
              curDown();
              break;
            case 'k': //Up
              curUp();
              break;
            case 'l': //Right
              curRight();
              break;

            default:
              break;
          }
        }
      }
    }


    void modeInput() {
      char c;
      while (currentState == State::INPUT) {
        read(STDIN_FILENO, &c, 1);
        switch (c) {
          case 27:
            handleEvent(Event::BACK);
            break;
        }
        cout << "Input mode" << endl;
      }
    }

    void modeCommand() {
      char c;
      while (currentState == State::COMMAND) {
        read(STDIN_FILENO, &c, 1);
        switch (c) {
          case 27:
            handleEvent(Event::BACK);
            break;
          case 'q':
            file->close();
            exit(0);
        }
        cout << "Command mode" << endl;
      }
    }

    void readFile(fstream* f) {
      file = f;
      file->seekg(0);
      string line;
      lines.clear();
      while (getline(*file, line)) {
        lines.push_back(line);
      }
    }

    void overwriteFile() {
      for(string line : lines) {
        
      }
    }

  private:
    fstream* file;
    vector<string> lines;
    int cX, cY;

    void curUp() {
      if (lines.empty()) return;
      if (cY > 0) {
        cX = std::min(cX, static_cast<int>(lines[cY-1].length() - 1));
        cY--;
      }
      else {
        cX = 0;
      }
      cout << "f\rx: " << cX << " y: " << cY;
    }

    void curDown() {
      if (lines.empty()) return;
      if (cY < lines.size() - 1) {
        cX = std::min(cX, static_cast<int>(lines[cY+1].length() - 1));
        cY++;
      }
      else {
        cX = lines.back().length() - 1;
      }
      cout << "f\rx: " << cX << " y: " << cY;
    }

    void curLeft() {
      if (cX > 0) cX--;
      cout << "f\rx: " << cX << " y: " << cY;
    }

    void curRight() {
      if (lines.empty()) return;
      if (cX < lines[cY].length() - 1) cX++;
      cout << "f\rx: " << cX << " y: " << cY;
    }

};

void dRawMode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &defSet);
}

void eRawMode() {
  tcgetattr(STDIN_FILENO, &defSet);
  atexit(dRawMode);

  struct termios raw = defSet;
  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}


/**
void modeNormal() {
	char c;
	while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
		if (iscntrl(c)) {
			printf("%d\n", c);
		} else {
			printf("%d ('%c')\n", c, c);
		}
	}
}
**/


void readFileAndShow(fstream* file) {
  char arr[sizeof(*file)];
  file->get(arr, sizeof(*file), EOF);
  cout << arr << endl;
}


int main(int argc, char** argv) {
	system("clear");
	eRawMode();

  string fileName;
  if (argc >= 2) fileName = argv[1];
  else {
    fileName = "fileNameNotGiven.txt";
  }

	fstream file;

	file.open(fileName, ios::in);

	if(file) {
		cout << "File found" << endl;
	}
	else {
		cout << "File " << fileName << " not found" << endl;
		ofstream{fileName};
		cout << "File " << fileName << " created" << endl;
		file.open(fileName, ios::in);
	}
	sleep(1);
	system("clear");

  readFileAndShow(&file);

  Lep lep;
  lep.readFile(&file);

  while(1) {
    switch (lep.currentState) {
      case State::INPUT:
        lep.modeInput();
        break;

      case State::COMMAND:
        lep.modeCommand();
        break;
  
      default:
        lep.modeNormal();      
        break;
    }
  }

	file.close();
	return 0;
}
