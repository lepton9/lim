#include <cstdlib>
#include <ios>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <stdlib.h> 
#include <fstream>
#include <termios.h>
#include <vector>
#include <algorithm>
#include <regex>
#include <sys/ioctl.h>
#include "../include/ModeState.h"
#include "../include/stev.h"
#include "../include/Clipboard.h"
#include "../include/Clip.h"
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

struct textArea {
  int bX, bY;
  int eX, eY;

  textArea() {
    clear();
  }

  void clear() {
    bX = -1;
    bY = -1;
    eX = -1;
    eY = -1;
  }

  bool isNull() {
      return (bX == -1 || bY == -1 || eX == -1 || bY == -1);
  }
};


class Lep : public ModeState {
  public:
    Lep() {
      unsaved = false;
      cX = 0;
      cY = 0;
      getWinSize();
      readConfig();
    }

    void modeNormal() {
      getWinSize();
      renderShownText(firstShownLine);
      updateStatBar();

      if (curIsAtMaxPos()) {
        cX = maxPosOfLine(cY);
        syncCurPosOnScr();
      }

      int c;
      while(currentState == State::NORMAL) {
        updateStatBar();
        c = readKey();
        //printf("%d ('%c')\n", c, c);
        switch (c) {
          case 27:
            handleEvent(Event::BACK);
            break;
          case 'i':
            handleEvent(Event::INPUT);
            break;
          case 'a':
            handleEvent(Event::INPUT);
            if (!curIsAtMaxPos()) curRight();
            break;
          case 'I':
            handleEvent(Event::INPUT);
            cX = minPosOfLineIWS(cY);
            syncCurPosOnScr();
            break;
          case 'A':
            handleEvent(Event::INPUT);
            cX = maxPosOfLine(cY);
            syncCurPosOnScr();
            break;
          case ':':
            handleEvent(Event::COMMAND);
            break;
          case 'v':
            handleEvent(Event::VISUAL);
            break;
          case 'V':
            handleEvent(Event::VLINE);
            break;
          case 'p':
            pasteClipboard();
            break;
          case 'g':
            c = readKey();
            if (c == 'g') {
              goToFileBegin();
            }
            break;
          case 'G':
            goToFileEnd();
            break;
          case 'd':
            c = readKey();
            if (c == 'd') {
              delCpLine();
            }
            break;
          case 'D':
            delCpLineEnd();
            break;
          case 'y':
            c = readKey();
            if (c == 'y') {
              cpLine();
            }
            break;
          case 'Y':
            cpLineEnd();
            break;
          case 14: // C-n
            // Show file tree
            break;
          case 16: // C-p
            c = readKey();
            if (isdigit(c)) {
              int d = c - 48; // 48 => 0
              //Paste cliboard[(d - 1 < 0) ? 9 : d - 1]
            }
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
      getWinSize();
      int c;
      while (currentState == State::INPUT) {
        updateStatBar();
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
      updateStatBar();

      while (currentState == State::COMMAND) {
        updateCommandBar();
        c = readKey();

        switch (c) {
          case 27:
            handleEvent(Event::BACK);
            syncCurPosOnScr();
            break;
          case 127: //Backspace
            comModeDelChar();
            break;
          case 10: //Enter
            if (execCommand()) {
              oldCommands.push_back(comLineText);
            }
            comLineText = "";
            handleEvent(Event::BACK);
            syncCurPosOnScr();
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
          syncCurPosOnScr();
        }
      }
    }

    void modeVisual() {
      if (currentState == State::VISUAL) {
        selectedText.bX = cX;
        selectedText.bY = cY;
        selectedText.eX = cX;
        selectedText.eY = cY;
      }
      else if (currentState == State::VLINE) {
        selectedText.bX = 0;
        selectedText.bY = cY;
        selectedText.eX = lines[cY].length();
        selectedText.eY = cY;
      }

      getWinSize();
      updateStatBar();
      int c;
      while (currentState == State::VISUAL || currentState == State::VLINE) {
        updateShowSelection();
        c = readKey();
        switch (c) {
          case 27: //Esc
            selectedText.clear();
            handleEvent(Event::BACK);
            break;
          case 10: //Enter
            break;
          case 127: //Backspace
            break;
          case DEL_KEY: //Delete
            deleteSelection();
            break;
          case 'y':
            copySelection();
            handleEvent(Event::BACK);
            break;
          case 'd':
            copySelection();
            deleteSelection();
            handleEvent(Event::BACK);
            break;
          case 'p':
            if (!clipboard.empty()) {
              deleteSelection();
              cX--;
              pasteClipboard();
            }
            handleEvent(Event::BACK);
            break;

          //Movement
          case 'h': case 'j': case 'k': case 'l':
          case LEFT_KEY: case DOWN_KEY: case UP_KEY: case RIGHT_KEY:
            curMove(c);
            updateSelectedText();
            break;

          default:
            break;
        }
      }
    }

    void start(string fName) {
      fileName = fName;
      readFile(fileName);
    }

  private:
    // Member function pointer type
    typedef void (Lep::*functionP)(string);
    struct func {
        string name;
        functionP f;
        func(string n, functionP fp) : name(n), f(fp) {}
    };

    int marginLeft = 7;
    int marginTop = 1;
    int winRows;
    int winCols;
    int firstShownLine = 0;
    vector<string> lines;
    bool unsaved;
    string comLineText = "";
    vector<string> oldCommands;
    vector<string> clipboard;
    textArea selectedText;
    int cX, cY;

    vector<char> validCommands = {'w', 'q', '!'};
    vector<func> functions = {
      func("rename", &Lep::rename), 
      func("restart", &Lep::restart), 
      func("set", &Lep::set), 
      func("setconfig", &Lep::setconfig)
    };

    string fileName;
    string configFilePath = "./.lepconfig";

    //Config
    int indentAm = 2;
    bool curWrap = true;
    int lineNumPad = 3;
    bool lineNum = true;
    bool relativenumber = false;
    int sBarColorBG;
    int sBarColorFG;

    bool readConfig() {
      fstream* conf = openFile(configFilePath, false);

      if (!lineNum) marginLeft = 2;

      delete conf;
      return true;
    }

    void setconfig(string path) {
      if (path == "") {
        //string newPath = queryUser("Set path to \".lepconfig\": ");
      }
      else {
        configFilePath = path;
      }
    }
    void rename(string fName) {
      if (fName == "") {
        string newName = queryUser("Set a new file name: ");

        if (comLineText == "") return;
        else {
          confirmRename(newName);
        }
      }
      else {
        confirmRename(fName);
      }
    }
    void set(string var) {
      cout << "set" << endl;
    }
    void restart(string p) {
      getWinSize();
      clsResetCur();
      readConfig();
      firstShownLine = 0;
      renderShownText(firstShownLine);
    }

    void confirmRename(string newName) {
      string ans = queryUser("Rename the file \"" + newName + "\"? [y/n]: ");
      if (ans == "y") {
        int result = std::rename(fileName.c_str(), newName.c_str());
        if (result == 0) {
          fileName = newName;
          showMsg("File renamed to \"" + newName + "\"");
          return;
        }
        else {
          string errMsg = getPerrorString("Error renaming file");
          showErrorMsg(errMsg);
          return;
        }
      }
      else if (ans == "n") {
        return;
      } else {
        confirmRename(newName);
        return;
      }
    }

    string queryUser(string query) {
      comLineText = query;
      updateStatBar(true);
      updateCommandBar();

      string ans = "";
      while(1) {
        char c = readKey();
        if (c == 27) {
          comLineText.clear();
          return "";
        }
        else if (c == 10) {
          return ans;
        }
        else if (c == 127 && ans.length() > 0) {
          comModeDelChar();
          ans.pop_back();
          updateCommandBar();
        }
        else if (!iscntrl(c) && c < 127) {
          ans += c;
          comLineText += c;
          updateCommandBar();
        }
      }
    }

    void inputChar(char c) {
      lines[cY].insert(cX, 1, c);
      cout << "\033[1C" << flush;
      cX++;
      updateRenderedLines(cY, 1);
    }

    void newlineEscSeq() {
      string newLine = lines[cY].substr(cX);
      lines[cY].erase(cX);
      cY++;
      lines.insert(lines.begin() + cY, newLine);
      cX = 0;
      if (cY-1 == firstShownLine + winRows - 2 - marginTop) {
        scrollDown();
        printf("\033[%dG", marginLeft);
      } else {
        printf("\033[1E\033[%dG", marginLeft);
      }
      fflush(stdout);
      updateRenderedLines(cY-1);
    }

    void tabKey() {
      lines[cY].insert(cX, string(indentAm, ' '));
      cX += indentAm;
      printf("\033[%dC", indentAm);
      fflush(stdout);
      updateRenderedLines(cY, 1);
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
        printf("\033[1A\033[%dG", cX + marginLeft);
        updateRenderedLines(cY);
      } else {
        lines[cY].erase(cX - 1, 1);
        cX--;
        printf("\033[1D");
        updateRenderedLines(cY, 1);
      }
      fflush(stdout);
    }

    void deleteKey() {
      if (cX == lines[cY].length()) {
        if (cY == lines.size()-1) return;
        string delLine = lines[cY+1];
        lines.erase(lines.begin() + cY + 1);
        lines[cY] += delLine;
        updateRenderedLines(cY);
      } else {
        lines[cY].erase(cX, 1);
        updateRenderedLines(cY, 1);
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
        getWinSize();
        if (ret == -1) {
          showErrorMsg(getPerrorString("Error reading key"));
          return '\0';
        }
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
      updateLineNums(firstShownLine);
    }

    void curUp() {
      if (lines.empty()) {
        return;
      }
      if (cY > 0) {
        if (cY == firstShownLine) {
          scrollUp();
        } else {
          cout << "\033[1A" << flush;
        }
        cY--;

        if (curIsAtMaxPos()) {
          cX = maxPosOfLine(cY);
          syncCurPosOnScr();
        }
      }
      else {
        cX = 0;
        syncCurPosOnScr();
      }
    }

    void curDown() {
      if (lines.empty()) {
        return;
      }
      if (cY < lines.size() - 1) {
        if (cY == firstShownLine + winRows - 2 - marginTop) {
          scrollDown();
        } else {
          cout << "\033[1B" << flush;
        }
        cY++;

        if (curIsAtMaxPos()) {
          cX = maxPosOfLine(cY);
          syncCurPosOnScr();
        }
      }
      else {
        cX = maxPosOfLine(cY);
        syncCurPosOnScr();
      }
    }

    void curLeft() {
      if (lines.empty()) {
        return;
      }
      if (cX > 0) {
        cout << "\033[1D" << flush;
        cX--;
      }
      else if (cX == 0 && curWrap && cY != 0) {
        if (cY == firstShownLine) {
          scrollUp();
        } else {
          printf("\033[1A");
        }
        cY--;
        cX = maxPosOfLine(cY);
        printf("\033[%dG", cX + marginLeft);
      }
    }

    void curRight() {      
      if (lines.empty()) {
        return;
      }
      if (!curIsAtMaxPos()) {
        cout << "\033[1C" << flush;
        cX++;
      }
      else if (curIsAtMaxPos() && curWrap && cY < lines.size() - 1) {
        if (cY == firstShownLine + winRows - 2 - marginTop) {
          scrollDown();
        } else {
          printf("\033[1E");
        }
        cY++;
        cX = 0;
        printf("\033[%dG", marginLeft);
      }
    }

    bool is_integer(const string &s){
        return regex_match(s, regex("[+-]?[0-9]+"));
    }

    unsigned charTOunsigned(const char * c) {
        unsigned unsignInt = 0;
        while (*c) {
            unsignInt = unsignInt * 10 + (*c - '0');
            c++;
        }
        return unsignInt;
    }

    int charToInt(const char * c) {
      return (*c == '-') ? -charTOunsigned(c+ 1) : charTOunsigned(c);
    }

    void goToFileBegin() {
      cY = 0;
      if (curIsAtMaxPos()) {
        cX = maxPosOfLine(cY);
      }
      firstShownLine = 0;
      renderShownText(firstShownLine);
      syncCurPosOnScr();
    }

    void goToFileEnd() {
      cY = lines.size() - 1;
      if (curIsAtMaxPos()) {
        cX = maxPosOfLine(cY);
      }
      firstShownLine = (cY < textAreaLength()) ? 0 : cY - textAreaLength();
      renderShownText(firstShownLine);
      syncCurPosOnScr();
    }

    void goToLine(int line) {
      if (line < 0) {
        line = 0;
      }
      else if (line >= lines.size()) {
        line = lines.size() - 1;
      }
      cY = line;
      if (curIsAtMaxPos()) {
        cX = maxPosOfLine(cY);
      }
      
      if (line < firstShownLine || line > firstShownLine + textAreaLength()) {
        int diff = firstShownLine - line;
        if (diff > 0 && diff <= 10) {
          firstShownLine = line;
        }
        else if (diff + textAreaLength() >= -10 && diff + textAreaLength() < 0) {
          firstShownLine = line - textAreaLength();
        }
        else {
          firstShownLine = line - ((textAreaLength())/2);
          if (firstShownLine < 0){
            firstShownLine = 0;
          }
          else if (firstShownLine + textAreaLength() > lines.size() - 1) {
            firstShownLine = lines.size() - 1 - (textAreaLength());
          }
        }
        renderShownText(firstShownLine);
      }
    }

    bool checkForFunctions(string func) {
      string f;
      string param;
      int spacePos = func.find(' ');
      if (spacePos != string::npos) {
        f = func.substr(0, spacePos);
        param = func.substr(spacePos + 1);
      } else {
        f = func;
        param = "";
      }
      for (const auto &function : functions) {
        if (function.name == f) {
          (this->*function.f)(param);
          sleep(1);
          return true;
        }
      }
      return false;
    }

    bool execCommand() {
      string com = comLineText.substr(1);
      if (is_integer(com)) {
        const char* clt = com.c_str();
        int line = charToInt(clt);
        goToLine(--line);
        return true;
      }

      if (checkForFunctions(com)) {
        comLineText = ':' + com;
        return true;
      }

      for(char c : comLineText) {
        if (c == ':') continue;
        if (find(validCommands.begin(), validCommands.end(), c) == validCommands.end()) {
          showErrorMsg("Not a valid command " + comLineText);
          return false;
        }
      }
      for(int i = 0; i < comLineText.length(); i++) {
        if (comLineText[i] == ':') continue;
        else if (comLineText[i] == 'w') overwriteFile();
        else if (comLineText[i] == 'q') {
          if (i < comLineText.length() - 1 && comLineText[i + 1] == '!') {
            exitLep(true);
          } else {
            exitLep(false);
          }
        }
      }
      if (comLineText.length() > 1) return true;
      return false;
    }

    int textAreaLength() {
      return winRows - marginTop - 2;
    }

    void showMsg(string msg) {
      updateStatBar(true);
      comLineText = msg;
      updateCommandBar();
      sleep(1);
    }

    void showErrorMsg(string error) {
      updateStatBar(true);
      comLineText = "\033[40;31m" + error + "\033[0m";
      updateCommandBar();
      sleep(1);
    }

    string getPerrorString(const string& errorMsg) {
        stringstream ss;
        ss << errorMsg << ": " << strerror(errno);
        return ss.str();
    }

    void clsResetCur() {
      cX = 0;
      cY = 0;
      printf("\033[2J\033[%d;%dH", marginTop, marginLeft);
      fflush(stdout);
    }

    void getCurPosOnScr(int* x, int* y) {
      cout << "\033[6n";
      char response[32];
      int bytesRead = read(STDIN_FILENO, response, sizeof(response) - 1);
      response[bytesRead] = '\0';
      sscanf(response, "\033[%d;%dR", x, y);
    }

    void syncCurPosOnScr() {
      printf("\033[%d;%dH", cY - firstShownLine + marginTop, cX + marginLeft);
      fflush(stdout);
    }
    
    bool curIsAtMaxPos() {
      if (lines[cY].empty()) return true;
      if (currentState == State::NORMAL && cX >= lines[cY].length() - 1) {
        return true;
      }
      else if (cX >= lines[cY].length()) {
        return true;
      }
      return false;
    }

    int maxPosOfLine(int y) {
      if (lines[y].empty()) return 0;
      return (currentState == State::NORMAL) ? lines[y].length() - 1 : lines[y].length();
    }

    int minPosOfLineIWS(int y) {
      if (lines[y].empty()) return 0;
      return lines[y].find_first_not_of(' ');
    }

    void scrollUp() {
      renderShownText(--firstShownLine);
    }

    void scrollDown() {
      renderShownText(++firstShownLine);
    }

    string alignR(string s, int w) {
      return string(w-s.length(), ' ') + s;
    }

    void checkLineNumSpace(string strLNum) {
      if (strLNum.length() > lineNumPad) {
        lineNumPad = strLNum.length();
        if (marginLeft - 1 <= lineNumPad) marginLeft++;
      }
    }

    string showLineNum(int lNum) {
      if (lineNum) {
        string strLNum = to_string(lNum);
        if (relativenumber) {
          if (lNum == 0) {
            checkLineNumSpace(to_string(cY+1));
            return "\033[0G\033[97m" + alignR(to_string(cY+1), lineNumPad) + "\033[0m"; // White highlight
          }
          return "\033[0G\033[90m" + alignR(strLNum, lineNumPad) + "\033[0m";
        }
        checkLineNumSpace(strLNum);
        if (lNum == cY + 1) {
          return "\033[0G\033[97m" + alignR(strLNum, lineNumPad) + "\033[0m"; // White highlight
        }
        return "\033[0G\033[90m" + alignR(strLNum, lineNumPad) + "\033[0m";
      }
      return "\0";
    }

    void updateLineNums(int startLine) {
      if (lineNum) {
        cout << "\033[s";
        printf("\033[%d;0H", marginTop + startLine - firstShownLine);
        if (relativenumber) {
          for(int i = startLine; i < lines.size() && i <= winRows-marginTop-2+startLine; i++) {
            cout << showLineNum(abs(cY-i)) << "\033[1E";
          }
        }
        else {
          for(int i = startLine; i < lines.size() && i <= winRows-marginTop-2+startLine; i++) {
            cout << showLineNum(i + 1) << "\033[1E";
          }
        }
        cout << "\033[u" << flush;
      }
    }

    void fillEmptyLines() {
      if (lines.size() < textAreaLength()) {
        for (int i = 0; i <= textAreaLength() - lines.size(); i++) {
          printf("\033[40;34m\033[2K\033[%dG%s\033[1E", 0, alignR("~", lineNumPad).c_str());
        }
        cout << "\033[0m";
      }
    }

    void updateRenderedLines(int startLine, int count = -1) {
      cout << "\033[s";
      printf("\033[%d;0H", marginTop + startLine - firstShownLine);
      if (count < 0) count = lines.size();

      int maxIter = min(textAreaLength() - (startLine - firstShownLine)
          , textAreaLength() + 1);
      int maxIterWithOffset = min(maxIter, textAreaLength() - (startLine - firstShownLine));

      for (int i = startLine; i < lines.size() && i <= startLine + maxIterWithOffset 
          && i - startLine < count; i++) {
        cout << "\033[2K";
        if (!relativenumber) cout << showLineNum(i + 1);
        printf("\033[%dG", marginLeft);
        cout << lines[i] << "\033[1E";
      }
      fillEmptyLines();
      cout << "\033[u" << flush;
      if (relativenumber) updateLineNums(firstShownLine);
    }

    void renderShownText(int startLine) {
      cout << "\033[s";
      printf("\033[%d;1H", marginTop);
      for(int i = startLine; i < lines.size() && i <= textAreaLength() + startLine; i++) {
        cout << "\033[2K";
        printf("\033[%dG", marginLeft);
        cout << lines[i] << "\033[1E";
      }
      fillEmptyLines();
      cout << "\033[u" << flush;
      updateLineNums(startLine);
    }

    void updateStatBar(bool showCommandLine = false) {
      cout << "\033[s"; // Save cursor pos
      printf("\033[%d;0H\033[2K\r", winRows - 1);
      if (currentState == State::COMMAND) {
        showCommandLine = true;
      }
      if (!showCommandLine) {
        printf("\033[%d;0H", winRows); // Move cursor to the last line
      }
      cout << "\033[2K\r"; // Clear current line
      cout << "\033[47;30m"; // Set bg color to white and text color to black
      string mode;
      if (currentState == State::INPUT) mode = " INPUT";
      else if (currentState == State::COMMAND) mode = " COMMAND";
      else if (currentState == State::VISUAL) mode = " VISUAL";
      else if (currentState == State::VLINE) mode = " V-LINE";
      else mode = " NORMAL";
      string saveText = " s ";
      string saveStatus;
      if (unsaved) saveStatus = "\033[41;30m" + saveText + "\033[47;30m";
      else saveStatus = "\033[42;30m" + saveText + "\033[47;30m";
      string eInfo = mode + " | " + fileName + " " + saveStatus;
      string curInfo = to_string(cY + 1) + ", " + to_string(cX + 1) + " ";
     
      int fillerSpace = winCols - eInfo.length() - curInfo.length() + (saveStatus.length() - saveText.length());

      if (fillerSpace < 0) {
        eInfo = mode + " | " + saveStatus;
        fillerSpace = winCols - eInfo.length() - curInfo.length() + (saveStatus.length() - saveText.length());
        fillerSpace = (fillerSpace < 0) ? 0 : fillerSpace;
      }

      cout << eInfo;
      cout << string(fillerSpace, ' ') << curInfo;
      cout << "\033[0m\033[u" << flush;
    }

    void updateCommandBar() {
      printf("\033[%d;1H", winRows); // Move cursor to the last line
      cout << "\033[2K\r"; // Clear current line
      cout << " " << comLineText;
      cout << "\033[0m" << flush;
    }

    void updateSelectedText() {
      if (currentState == State::VISUAL) {
        selectedText.eX = cX;
        selectedText.eY = cY;
      }
      else if (currentState == State::VLINE) {
        selectedText.eX = lines[cY].length();
        selectedText.eY = cY;
      }
    }

    void updateShowSelection() {
      if (selectedText.isNull()) return;
      textArea sel = selectedText;
      checkSelectionPoints(&sel);

      if (sel.bY < firstShownLine) {
        sel.bY = firstShownLine;
        sel.bX = 0;
      }
      if (sel.eY > firstShownLine + textAreaLength()) {
        sel.eY = firstShownLine + textAreaLength();
        sel.eX = lines[sel.eY].length();
      }

      updateRenderedLines((sel.bY == firstShownLine) ? firstShownLine : sel.bY-1, sel.eY-sel.bY + 3);

      cout << "\033[s\033[7m"; // Inverse color
      printf("\033[%d;%dH", marginTop + sel.bY - firstShownLine, marginLeft + sel.bX);
      if (sel.bY == sel.eY) {
        cout << lines[sel.bY].substr(sel.bX, sel.eX-sel.bX + 1);
      } else {
        cout << lines[sel.bY].substr(sel.bX) << "\033[1E";
        for (int i = sel.bY + 1; i < sel.eY; i++) {
          printf("\033[%dG", marginLeft);
          cout << lines[i] << "\033[1E";
        }
        printf("\033[%dG", marginLeft);
        cout << lines[sel.eY].substr(0, sel.eX + 1);
      }
      cout << "\033[0m\033[u" << flush;
    }

    void copySelection() {
      if (selectedText.isNull()) return;
      checkSelectionPoints(&selectedText);
      clipboard.clear();

      int startX = selectedText.bX;
      int endX = selectedText.eX;
      for (int i = selectedText.bY; i <= selectedText.eY; i++) {
        int len = lines[i].length() - startX;
        if (i == selectedText.eY) {
          len = (endX == lines[i].length()) ? lines[i].length() - startX : endX - startX + 1;
        }
        clipboard.push_back(lines[i].substr(startX, len));
        startX = 0;
      }
    }

    void deleteSelection() {
      if (selectedText.isNull()) return;
      unsaved = true;
      checkSelectionPoints(&selectedText);
      
      int startLine = selectedText.bY;
      int endLine = selectedText.eY;
      
      if (startLine == endLine)  {
        if (lines[startLine].length() == selectedText.eX - selectedText.bX) {
          lines.erase(lines.begin() + startLine);
        } else {
          if (selectedText.eX == lines[startLine].length()) {
            lines[startLine].erase(selectedText.bX);
            lines[startLine].append(lines[startLine + 1]);
            lines.erase(lines.begin() + startLine + 1);
          } else {
            lines[startLine].erase(selectedText.bX, selectedText.eX - selectedText.bX + 1);
          }
        }
      } else {
        int delLines = 0;
        lines[startLine].erase(selectedText.bX);
        if (endLine - startLine > 1) {
          lines.erase(lines.begin() + startLine + 1, lines.begin() + endLine);
          delLines = endLine - startLine - 1;
        }

        if (selectedText.eX == lines[endLine - delLines].length()) {
          lines.erase(lines.begin() + endLine - delLines);
        } else {
          lines[endLine - delLines].erase(0, selectedText.eX + 1);
        }
        lines[startLine].append(lines[endLine - delLines]);
        lines.erase(lines.begin() + endLine - delLines);
      }

      if (lines[cY].empty()) cX = 0;
      else {
        cX = min(selectedText.bX, static_cast<int>(lines[startLine].length()));
      }
      cY = selectedText.bY;
      syncCurPosOnScr();
      selectedText.clear();
      updateRenderedLines(startLine);
    }
    
    void checkSelectionPoints(textArea* selection) {
      if (currentState == State::VISUAL) {
        if (selection->eY < selection->bY || 
            (selection->bY == selection->eY && selection->eX < selection->bX)) {
          swap(selection->bY, selection->eY);
          swap(selection->bX, selection->eX);
        }
      }
      else if (currentState == State::VLINE) {
        if (selection->eY < selection->bY) {
          swap(selection->bY, selection->eY);
          selection->bX = 0;
          selection->eX = lines[selection->eY].length();
        }
      }
    }

    void cpLine() {
      clipboard.clear();
      clipboard.push_back(lines[cY]);
    }

    void cpLineEnd() {
      clipboard.clear();
      clipboard.push_back(lines[cY].substr(cX));
    }

    void delCpLine() {
      unsaved = true;
      cpLine();
      lines.erase(lines.begin() + cY);
      updateRenderedLines(cY);
      if (curIsAtMaxPos()) {
        cX = maxPosOfLine(cY);
        syncCurPosOnScr();
      }
    }

    void delCpLineEnd() {
      unsaved = true;
      cpLineEnd();
      lines[cY].erase(cX);
      updateRenderedLines(cY, 1);
      if (cX > 0) {
        cX--;
        syncCurPosOnScr();
      }
    }

    void pasteClipboard() {
      if (clipboard.empty()) return;
      unsaved = true;
      int begY = cY;

      string remain = "";
      for (string line : clipboard) {
        if (lines[cY].empty()) {
          lines[cY] = line;
          cX = (line == "") ? 0 : maxPosOfLine(cY);
        }
        else if (curIsAtMaxPos()) {
          lines.insert(lines.begin() + cY + 1, line);
          cY++;
          cX = maxPosOfLine(cY);
        }
        else {
          remain = lines[cY].substr(cX + 1);
          lines[cY].erase(cX + 1);
          lines[cY].append(line);
          cX = maxPosOfLine(cY);
        }
      }
      lines[cY].append(remain);

      if (cY - firstShownLine > textAreaLength()) {
        firstShownLine = cY - winRows + marginTop + 2;
        renderShownText(firstShownLine);
      }
      else if (cY == begY) {
        updateRenderedLines(begY, 1);
      } else {
        updateRenderedLines(begY);
      }
      syncCurPosOnScr();
    }

    void getWinSize() {
      struct winsize size;
      if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == -1) {
        showErrorMsg(getPerrorString("Failed to get terminal window size"));
        return;
      }
      if (size.ws_row != winRows || size.ws_col != winCols) {
        winRows = size.ws_row;
        winCols = size.ws_col;
        renderShownText(firstShownLine);
        updateStatBar();
      }
    }

    fstream* openFile(string fName, bool createIfNotFound) {
      fstream* file = new fstream(fileName);

      if(file->is_open()){
        showMsg("File \"" + fileName + "\" found");
      }
      else if (createIfNotFound) {
        ofstream* file = new ofstream(fileName);
        showErrorMsg("File \"" + fileName + "\" created");
        file->open(fileName, ios::in);
      }

      return file;
    }

    void readFile(string fName) {
      fileName = fName;
      fstream* fptr = openFile(fileName, true);
      readToLines(fptr);
      delete fptr;
      syncCurPosOnScr();
    }

    void readToLines(fstream *file) {
      lines.clear();
      if (file->peek() == EOF) {
        lines.push_back("");
      } else {
        string line;
        while (getline(*file, line)) {
          lines.push_back(line);
        }
      }
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

    void exitLep(bool force) {
      if (unsaved && !force) {
        while(1) {
          string ans = queryUser("Unsaved changes. Save changes [y/n]:");
          if (ans == "y") {
            overwriteFile();
            break;
          }
          else if (ans == "n") break;
          else if (ans == "" && comLineText.empty()) return;
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
  lep.start(fileName);

  while(1) {
    switch (lep.currentState) {
      case State::INPUT:
        lep.modeInput();
        break;

      case State::COMMAND:
        lep.modeCommand();
        break;

      case State::VLINE:
      case State::VISUAL:
        lep.modeVisual();
        break;
  
      default:
        lep.modeNormal();      
        break;
    }
  }

	return 0;
}
