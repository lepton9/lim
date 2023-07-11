#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <ios>
#include <iostream>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <fstream>
#include <termios.h>
#include <vector>
#include <algorithm>
#include <locale>
#include <codecvt>
#include <sys/ioctl.h>
#include "../include/ModeState.h"
#include "../include/stev.h"
//#include <conio.h>


using namespace std;

//Original terminal settings 
struct termios defSet;

enum sKey {
  UP_KEY = 400,
  LEFT_KEY,
  RIGHT_KEY,
  DOWN_KEY,
  DEL_KEY,
};

class Lep : public ModeState {
  public:
    Lep() {
      unsaved = false;
      cX = 0;
      cY = 0;
      getWinSize();
    }

    void modeNormal() {
      int c;
      while(currentState == State::NORMAL) {
        refresh();
        c = readKey();
        //printf("%d ('%c')\n", c, c);
        switch (c) {
          case 27:
            //cout << "esc" << endl;
            handleEvent(Event::BACK);
            break;
          case 'i':
            //cout << "i" << endl;
            handleEvent(Event::INPUT);
            break;
          case ':':
            //cout << ":" << endl;
            handleEvent(Event::COMMAND);
            break;

          //Movement
          case 'h': case 'j': case 'k': case 'l':
          case LEFT_KEY: case DOWN_KEY: case UP_KEY: case RIGHT_KEY:
            curMove(c);
            break;
        }
      }
    }

    void modeInput() {
      int c;
      while (currentState == State::INPUT) {
        refresh();
        c = readKey();
        switch (c) {
          case 27: //Esc
            handleEvent(Event::BACK);
            break;
          case 10: //Enter
            unsaved = true;
            newlineEscSeq();
            break;
          case 9: //Tab
            unsaved = true;
            tabKey();
            break;
          case 127: //Backspace
            unsaved = true;
            backspaceKey();
            break;
          case DEL_KEY: //Delete
            unsaved = true;
            deleteKey();
            break;

          //Movement
          case LEFT_KEY: case DOWN_KEY: case UP_KEY: case RIGHT_KEY:
            curMove(c);
            break;

          default:
            if (!iscntrl(c)) {
              unsaved = true;
              inputChar(c);
            }
            break;
        }
      }
    }

    void modeCommand() {
      int c;
      int comInd = oldCommands.size();
      string curCom = "";
      comLineText = ":";
      while (currentState == State::COMMAND) {
        refresh();
        c = readKey();

        switch (c) {
          case 27:
            handleEvent(Event::BACK);
            break;
          case 127: //Backspace
            comModeDelChar();
            break;
          case 10: //Enter
            if (execCommand()) {
              oldCommands.push_back(comLineText);
            }
            handleEvent(Event::BACK);
            break;
          case UP_KEY:
            if (curCom == "") curCom = comLineText;
            if (comInd > 0) {
              comInd--;
              comLineText = oldCommands[comInd];
            }
            break;
          case DOWN_KEY:
            if (comInd < oldCommands.size()) {
              comInd++;
            }
            if (comInd == oldCommands.size()) {
              comLineText = curCom;
              break;
            }
            comLineText = oldCommands[comInd];

            break;
          default:
            if (!iscntrl(c) && c < 127) {
              comLineText += c;
              curCom = comLineText;
              comInd = oldCommands.size();
            }
            break;
        }
        if (comLineText.empty()) {
          handleEvent(Event::BACK);
        }
      }
    }

    void readFile(string fName) {
      fileName = fName;
      fstream file(fileName);

      if(file) {
        cout << "File found" << endl;
      }
      else {
        cout << "File " << fileName << " not found" << endl;
        ofstream file(fileName);
        cout << "File " << fileName << " created" << endl;
        file.open(fileName, ios::in);
      }
      sleep(1);
      system("clear");

      if (file.peek() == EOF) {
        lines.push_back("");
      } else {
        string line;
        while (getline(file, line)) {
          lines.push_back(line);
        }
      }
      file.close();
    }
      
    void overwriteFile() {
      if (lines.empty() || (lines.size() == 1 && lines.front().empty())) {
        unsaved = false;
        return;
      } 
      ofstream file(fileName);
      for(const string& line : lines) {
        file << line << '\n';
      }
      file.close();
      unsaved = false;
    }

  private:
    int marginLeft = 2;
    int marginTop = 1;
    int winRows;
    int winCols;
    string fileName;
    vector<string> lines;
    bool unsaved;
    vector<char> validCommands = {'w', 'q'};
    string comLineText = "";
    vector<string> oldCommands;
    int cX, cY;

    //Config
    int indentAm = 2;
    bool curWrap = false;
    int sBarColorBG;
    int sBarColorFG;

    void inputChar(char c) {
      lines[cY].insert(cX, 1, c);
      //if(lines[cY][cX-1] < 0 && c < 0) return;
      cX++;
    }

    void newlineEscSeq() {
      string newLine = lines[cY].substr(cX);
      lines[cY].erase(cX);
      lines.insert(lines.begin() + cY + 1, newLine);
      cY++;
      cX = 0;
    }

    void tabKey() {
      lines[cY].insert(cX, string(indentAm, ' '));
      cX += indentAm;
    }

    void backspaceKey() {
      int ePos;
      if (cX == 0) {
        if (cY == 0) return;
        string delLine = lines[cY];
        lines.erase(lines.begin() + cY, lines.begin() + cY + 1);
        cY--;
        cX = lines[cY].length();
        lines[cY].append(delLine);

      } else {
        lines[cY].erase(cX - 1, 1);
        cX--;
      }
    }

    void deleteKey() {
      if (cX == lines[cY].length()) {
        if (cY == lines.size()-1) return;
        string delLine = lines[cY+1];
        lines.erase(lines.begin() + cY + 1);
        lines[cY] += delLine;

      } else {
        lines[cY].erase(cX, 1);
      }
    }

    void comModeDelChar() {
      if (!comLineText.empty()) {
        comLineText.pop_back();
      }
    }

    int readKey() {
      int ret;
      char c;
      while ((ret = read(STDIN_FILENO, &c, 1)) != 1) {
        if (ret == -1) return '\0';
      }

      if (c == '\x1b') {
        char keySeq[3];
        if (read(STDIN_FILENO, &keySeq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &keySeq[1], 1) != 1) return '\x1b';

        if (keySeq[0] == '[') {

          if (keySeq[1] >= '0' && keySeq[1] <= '9') {
            if (read(STDIN_FILENO, &keySeq[2], 1) != 1) return '\x1b';
            if (keySeq[2] == '~') {
              switch (keySeq[1]) {
                case '3': return DEL_KEY;
              }
            }
          }
          else {
            switch (keySeq[1]) {
              case 'A': return UP_KEY;
              case 'B': return DOWN_KEY;
              case 'C': return RIGHT_KEY;
              case 'D': return LEFT_KEY;
            }
          }
        }
        return '\x1b';
      }
      else if (c < 0) {
        return '\0';
      } else {
        return c;
      }
    }

    void curMove(int c) {
      if (c == 'h' || c == LEFT_KEY) {
        curLeft();
      } else if (c == 'j' || c == DOWN_KEY) {
        curDown();
      } else if (c == 'k' || c == UP_KEY) {
        curUp();
      } else if (c == 'l' || c == RIGHT_KEY) {
        curRight();
      }
      updateStatBar();
    }

    void curUp() {
      if (lines.empty()) {
        return;
      }
      if (cY > 0) {
        cX = std::min(cX, static_cast<int>(lines[cY-1].length()));
        cY--;
      }
      else {
        cX = 0;
      }
    }

    void curDown() {
      if (lines.empty()) {
        return;
      }
      if (cY < lines.size() - 1) {
        cX = std::min(cX, static_cast<int>(lines[cY+1].length()));
        cY++;
      }
      else {
        cX = lines.back().length();
      }
    }

    //Maybe in config file state if you want to wrap or not wrap = true/false
    void curLeft() {
      if (lines.empty()) {
        return;
      }
      if (cX > 0) cX--;
      else if (cX == 0 && curWrap && cY != 0) {
        cY--;
        cX = lines[cY].length();
      }
    }

    //Change to go to next line beginning if cX == lines[cY].length()
    void curRight() {      if (lines.empty()) {
        return;
      }
      if (cX < lines[cY].length()) cX++;
      else if (cX == lines[cY].length() && curWrap && cY != lines.size() - 1) {
        cY++;
        cX = 0;
      }
    }

    bool execCommand() {
      for(char c : comLineText) {
        if (c == ':') continue;
        if (find(validCommands.begin(), validCommands.end(), c) == validCommands.end()) {
          showErrorMsg("Not a valid command " + comLineText);
          return false;
        }
      }
      for(char ch : comLineText) {
        if (ch == ':') continue;
        else if (ch == 'w') overwriteFile();
        else if (ch == 'q') exitLep();
      }
      if (comLineText.length() > 1) return true;
      return false;
    }

    void showErrorMsg(string error) {
      comLineText = "\033[40;31m" + error + "\033[0m";
      updateCommandBar();
      sleep(1);
    }

    void updateCurPosOnScr() {
      cout << "\033[H";
      printf("\033[%dC", cX + marginLeft);
      printf("\033[%dB", cY + marginTop);
      fflush(stdout);
    }

    void renderShownText() {
      cout << "\033[H";
      printf("\033[%dB", marginTop);
      for(string line : lines) {
        printf("\033[%dC", marginLeft);
        cout << line << "\033[1E";
      }
      if (lines.size() < winRows) {
        for (int i = 0; i < winRows-lines.size(); i++) {
          printf("\033[40;34m\033[%dC~\033[1E", marginLeft);
        }
        cout << "\033[0m" << flush;
      }
      //"\x1b[K" erases the end of the line
    }

    void updateStatBar() {
      cout << "\033[s"; // Save cursor pos
      cout << "\033[" << winRows + marginTop + 1 << ";1H" << "\033[2K\r";
      if (currentState != State::COMMAND) {
        cout << "\033[" << winRows + marginTop + 2 << ";1H"; // Move cursor to the last line
      }
      cout << "\033[2K\r"; // Clear current line
      cout << "\033[47;30m"; // Set bg color to white and text color to black
      string mode;
      if (currentState == State::INPUT) mode = " INPUT";
      else if (currentState == State::COMMAND) mode = " COMMAND";
      else mode = " NORMAL";
      string saveText = " s ";
      string saveStatus;
      if (unsaved) saveStatus = "\033[41;30m" + saveText + "\033[47;30m";
      else saveStatus = "\033[42;30m" + saveText + "\033[47;30m";
      string eInfo = mode + " | " + fileName + " " + saveStatus;
      string curInfo = " | L: " + to_string(cY+1) + " C: " + to_string(cX+1) + " ";
      cout << eInfo;
     
      int fillerSpace = winCols - eInfo.length() - curInfo.length() + (saveStatus.length() - saveText.length());
      cout << string(fillerSpace, ' ') << curInfo;
      // Restore text color and cursor pos
      cout << "\033[0m\033[u" << flush;
    }

    void updateCommandBar() {
      if (currentState != State::COMMAND) return;
      updateStatBar();
      cout << "\033[s"; // Save cursor pos
      cout << "\033[" << winRows + marginTop + 2 << ";1H"; // Move cursor to the last line
      cout << "\033[2K\r"; // Clear current line
      cout << " " << comLineText;
      cout << "\033[0m\033[u" << flush;
    }

    void refresh() {
      getWinSize();
      cout << "\x1b[2J"; //Clears the screen
      renderShownText();
      updateStatBar();
      updateCommandBar();
      updateCurPosOnScr();
    }

    void getWinSize() {
      struct winsize size;
      if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == -1) {
          cerr << "Failed to get terminal window size" << endl;
          return;
      }
      winRows = size.ws_row - marginTop - 2; //Stat bar and command line
      winCols = size.ws_col - marginLeft;
      //cout << "Terminal window size: " << size.ws_col << " columns x " << size.ws_row << " rows" << endl;
    }

    void exitLep() {
      if (unsaved) {
        comLineText = "Unsaved changes. Save changes [y/n]:";
        string ans = "";
        refresh();
        while(1) {
          char c = readKey();
          if (c == 10) {
            if (ans == "y") {
              overwriteFile();
              break;
            }
            else if (ans == "n") break;
          }
          else if (c == 127 && ans.length() > 0) {
            comModeDelChar();
            ans.pop_back();
            updateCommandBar();
          }
          else if (c == 27) {
            comLineText.clear();
            handleEvent(Event::BACK);
            return;
          }
          else if (!iscntrl(c) && c < 127) {
            ans += c;
            comLineText += c;
            updateCommandBar();
          }
        }
      }
      system("clear");
      exit(0);
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
  raw.c_cc[VTIME] = 1; //Timeout time in 1/10 of seconds
  raw.c_cc[VMIN] = 0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main(int argc, char** argv) {
	system("clear");
	eRawMode();

  string fileName;
  if (argc >= 2) fileName = argv[1];
  else {
    fileName = "fileNameNotGiven.txt";
  }


  Lep lep;
  lep.readFile(fileName);

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

	return 0;
}
