#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <cstring>
#include <string>
#include <unistd.h>
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
#include "../include/Config.h"
#include "../include/Filetree.h"
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

struct pos {
  int x, y;
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
      updateStatBar();

      if (fileOpen && curIsAtMaxPos()) {
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
            if (matchesHighlighted()) {
              resetMatchSearch();
            }
            handleEvent(Event::BACK);
            break;
          case 10:
            if (curInFileTree) {
              fTreeSelect();
            }
            break;
          case 'i':
            handleEvent(Event::INPUT);
            break;
          case 'a':
            if (curInFileTree) {
              createFile();
              break;
            }
            handleEvent(Event::INPUT);
            if (!curIsAtMaxPos()) curRight();
            break;
          case 'r': //TODO: rename file on cur
            if (curInFileTree) {
              renameFileOnCur();
            }
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
          case '!':
            handleEvent(Event::COMMAND);
            comLineText = ":!";
            execBashCommand();
            restart("");
            handleEvent(Event::BACK);
            break;
          case '/':
          case 6: // C+f
            handleEvent(Event::COMMAND);
            search();
            handleEvent(Event::BACK);
            break;
          case '*':
            if (curInFileTree) break;
            searchStrOnCur();
            break;
          case 'n':
            if (matchesHighlighted()) {
              gotoNextMatch();
            }
            break;
          case 'N':
            if (matchesHighlighted()) {
              gotoLastMatch();
            }
            break;
          case 'v':
            handleEvent(Event::VISUAL);
            break;
          case 'V':
            handleEvent(Event::VLINE);
            break;
          case 'c':
            if (curInFileTree) {
              copyFileOnCur();
            }
            break;
          case 'p':
            if (curInFileTree) {
              pasteFileInCurDir();
              break;
            }
            pasteClipboard();
            break;
          case 'W': // Beginning of next word
          case 'w':
            gotoBegOfNext();
            break;
          case 'E': // End of current word or next if at end of word
          case 'e':
            gotoEndOfNext();
            break;
          case 'B': // Beginning of last word
          case 'b':
            gotoBegOfLast();
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
            if (curInFileTree) {
              removeFileOnCur();
              break;
            }
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
            ftree.toggleShow();
            renderFiletree();
            break;
          case 8: // C-h
            if (ftree.isShown() && !curInFileTree) {
              curInFileTree = true;
              syncCurPosOnScr();
            }
            break;
          case 12: // C-l
            if (ftree.isShown() && curInFileTree) {
              curInFileTree = false;
              syncCurPosOnScr();
            }
            break;
          case 16: // C-p + #, paste from clipboard at index #
            c = readKey();
            if (isdigit(c)) {
              int d = c - 48; // 48 => 0
              pasteClipboard((d - 1 < 0) ? 9 : d - 1);
            }
            break;

          //Movement
          case 'h': case 'j': case 'k': case 'l':
          case LEFT_KEY: case DOWN_KEY: case UP_KEY: case RIGHT_KEY:
            if (curInFileTree) curMoveFileTree(c);
            else curMove(c);
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
      path = filesystem::current_path();
      ftree = Filetree(path);
      if (fName == "") {
        curInFileTree = true;
        ftree.toggleShow();
        renderFiletree();
        showMsg(":lep");
        printStartUpScreen();
      } else {
        readFile(fName, true);
      }
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
    int lineNumPad = 3;
    int winRows;
    int winCols;
    int firstShownLine = 0;
    int firstShownFile = 0;
    vector<string> lines;
    bool unsaved;

    string searchStr;
    vector<pos> matches;
    int curMatch = -1;

    bool fileOpen = false;
    bool curInFileTree = false;
    Filetree ftree;

    string comLineText = "";
    vector<string> oldCommands;
    Clipboard clipboard;
    textArea selectedText;
    int cX, cY;

    vector<char> validCommands = {'w', 'q', '!'};
    vector<func> functions = {
      func("rename", &Lep::rename), 
      func("restart", &Lep::restart), 
      func("set", &Lep::set), 
      func("setconfig", &Lep::setconfig),
      func("path", &Lep::showPath),
    };

    string path;
    string fileName;
    string fullpath() {
        return path + "/" + fileName;
    }

    Config config;

    bool readConfig() {
      config.parse();

      updateVariables();

      return true;
    }

    void clearAll() {
      path = "";
      fileName = "";
      searchStr = "";
      comLineText = "";
      matches.clear();
      fileOpen = false;
      firstShownLine = 0;
      firstShownFile = 0;
      curMatch = -1;
      lines.clear();
      unsaved = false;
      clipboard.clear();
      oldCommands.clear();
      selectedText.clear();
    }

    void updateVariables() {
      if (config.lineNum) marginLeft = 7;
      else marginLeft = 2;
    }

    void setconfig(string path) {
      string newPath;
      bool success = false;
      if (path == "") {
        newPath = queryUser("Set path to \".lepconfig\": ");
        if (newPath == "") return;
      }
      else {
        newPath = path;
      }
      success = config.setFilePath(newPath);
      if (success) {
        showMsg("Path set to " + newPath);
        updateVariables();
        renderShownText(firstShownLine);
      } else {
        showErrorMsg("Not a valid path " + newPath);
      }
    }

    void rename(string fName) {
      if (fName == "") {
        string newName = queryUser("Set a new file name: ");

        if (newName == "") return;
        else {
          confirmRename(newName);
        }
      }
      else {
        confirmRename(fName);
      }
    }

    void set(string var) {
      if (var == "") {
        return;
      }
      bool success = config.set(var);
      if (success) {
        updateVariables();
        renderShownText(firstShownLine);
        showMsg("Updated " + var);
      }
      else {
        showErrorMsg("Unknown option: " + var);
      }
    }

    void restart(string p) {
      getWinSize();
      clsResetCur();
      readConfig();
      firstShownLine = 0;
      firstShownFile = 0;
      if (ftree.isShown()) renderFiletree();
      renderShownText(firstShownLine);
      syncCurPosOnScr();
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

    void showPath(string args) {
      showMsg(fullpath());
      syncCurPosOnScr();
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
      int padLeft = (ftree.isShown()) ? ftree.treeWidth + 1 : 0;
      string newLine = lines[cY].substr(cX);
      lines[cY].erase(cX);
      cY++;
      lines.insert(lines.begin() + cY, newLine);
      cX = 0;
      if (cY-1 == firstShownLine + winRows - 2 - marginTop) {
        scrollDown();
        printf("\033[%dG", marginLeft + padLeft);
      } else {
        printf("\033[1E\033[%dG", marginLeft + padLeft);
      }
      fflush(stdout);
      updateRenderedLines(cY-1);
    }

    void tabKey() {
      lines[cY].insert(cX, string(config.indentAm, ' '));
      cX += config.indentAm;
      printf("\033[%dC", config.indentAm);
      fflush(stdout);
      updateRenderedLines(cY, 1);
    }

    void backspaceKey() {
      int padLeft = (ftree.isShown()) ? ftree.treeWidth + 1 : 0;
      int ePos;
      if (cX == 0) {
        if (cY == 0) return;
        string delLine = lines[cY];
        lines.erase(lines.begin() + cY, lines.begin() + cY + 1);
        cY--;
        cX = lines[cY].length();
        lines[cY].append(delLine);
        printf("\033[1A\033[%dG", cX + marginLeft + padLeft);
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

    void printStartUpScreen() {
      cout << "\033[s";
      vector<string> logo = {
          " ____",                  
          "|    |    ____ ______  ",
          "|    |  _/ __ \\\\____ \\ ",
          "|    |__\\  ___/|  |_) |",
          "|_______ \\___      __/ ",
          "               |__|    "
      };

      printf("\033[%d;0H", marginTop);
      for (vector<string>::iterator it = logo.begin(); it < logo.end(); it++) {
        printf("\033[%dG", ftree.treeWidth + 5);
        printf("%s\033[1E", it->c_str());
      }
      cout << "\033[u";
    }

    void fTreeSelect() {
      if (!curInFileTree) return;

      if (ftree.getElementOnCur()->isDir) {
        firstShownFile = 0;
        showMsg(ftree.getElementOnCur()->path);
        ftree.changeDirectory(ftree.getElementOnCur()->path);
        renderFiletree();
        syncCurPosOnScr();
      } else {
        if (unsaved) {
          while(1) {
            string ans = queryUser("Unsaved changes. Save changes [y/n]:");
            if (ans == "y") {
              overwriteFile();
              break;
            }
            else if (ans == "n") {
              unsaved = false;
              break;
            }
            else if (ans == "" && comLineText.empty()) return;
          }
        }
        path = ftree.getElementOnCur()->path.parent_path();
        readFile(ftree.getElementOnCur()->name);
        firstShownLine = 0;
        cX = 0, cY = 0;
        curInFileTree = false;
        syncCurPosOnScr();
        renderShownText(firstShownLine);
      }
    }

    void curMoveFileTree(int c) {
      int lastLine = ftree.cY;
      if (c == 'h' || c == LEFT_KEY) {
        //curLeftTree();
      } else if (c == 'j' || c == DOWN_KEY) {
        curTreeDown();
      } else if (c == 'k' || c == UP_KEY) {
        curTreeUp();
      } else if (c == 'l' || c == RIGHT_KEY) {
        //curRightTree();
      }
      if (ftree.cY != lastLine) {
        updateFTreeHighlight(ftree.cY, lastLine);
        syncCurPosOnScr();
      }
    }

    void refreshFileTree() {
      ftree.refresh();
      renderFiletree();
      syncCurPosOnScr();
    }

    void createFile() {
      if (!curInFileTree) return;

      string fName = queryUser("Create file " + path + "/");
      if (fName == "") {
        comLineText = "";
        updateCommandBar();
        return;
      }

      string fullPath = ftree.current_path().string() + "/" + fName;
      fstream* newFile = openFile(fullPath, true);
      newFile->close();
      delete newFile;
      refreshFileTree();
    }

    void closeCurFile() {
      clearAll();
      curInFileTree = true;
      if (!ftree.isShown()) ftree.toggleShow();
      clsResetCur();
      renderFiletree();
      printStartUpScreen();
    }

    void removeDirOnCur() {
      string dPathRem = ftree.getElementOnCur()->path;
      string ans = queryUser("Delete directory and its contents \"" + dPathRem + "\"? (y/n): ");
  
      if (ans == "y") {
        int c = filesystem::remove_all(dPathRem);
        if (dPathRem == path) {
          closeCurFile();
        }
        refreshFileTree();
        showMsg(to_string(c) + " files or directories deleted");
        syncCurPosOnScr();
      }
      else {
        comLineText = "";
        updateCommandBar();
      }
    }

    void removeFileOnCur() {
      if (!curInFileTree) return;
      if (ftree.getElementOnCur()->isDir) {
        removeDirOnCur();
        return;
      }
      string fPathRem = ftree.getElementOnCur()->path;
      string fNameRem = ftree.getElementOnCur()->name;
      string ans = queryUser("Remove \"" + fNameRem + "\"? (y/n): ");
      if (ans == "y") {
        filesystem::remove(fPathRem);
        refreshFileTree();
        showMsg("File " + fPathRem + " was removed");
        syncCurPosOnScr();
        if (fPathRem == fullpath()) {
          closeCurFile();
        }
      }
      else {
        comLineText = "";
        updateCommandBar();
      }
    }

    void renameFileOnCur() {
      if (!curInFileTree) return;
      
      string newName = queryUser("Rename " + ftree.getElementOnCur()->name + " to ");
      if (newName == "") return;
      string oldPath = ftree.getElementOnCur()->path.string();
      string newPath = oldPath.substr(0, oldPath.find_last_of("/") + 1) + newName;
      filesystem::rename(ftree.getElementOnCur()->path, newPath);
      if (oldPath == fullpath()) {
        path = ftree.current_path();
        fileName = newName;
        updateStatBar();
      }
      refreshFileTree();
      showMsg(oldPath + " -> " + newPath);
      syncCurPosOnScr();
    }

    void copyFileOnCur() {
      if (!curInFileTree) return;
      ftree.copy();
      if (ftree.copied->isDir) {
        showMsg("Directory " + ftree.copied->path.string() + " copied recursively");
      } else {
        showMsg("File " + ftree.copied->path.string() + " copied");
      }
      syncCurPosOnScr();
    }

    void pasteFileInCurDir() {
      if (!curInFileTree || ftree.copied == NULL) return;
      string oldPath = ftree.copied->path.string();
      string newPath = ftree.current_path().string() + "/" + ftree.copied->name;
      if (ftree.copied->path.parent_path() == ftree.current_path()) {
        string newName = queryUser("Rename to " + ftree.current_path().parent_path().string() + "/");
        if (newName == "") return;
        newPath = ftree.current_path().string() + "/" + newName;
      }
      ftree.paste(newPath);

      refreshFileTree();
      showMsg(oldPath + " copied to " + newPath);
      syncCurPosOnScr();
    }

    void curTreeUp() {
      if (ftree.cY <= 0) return;
      if (ftree.cY == firstShownFile && firstShownFile > 0) {
        firstShownFile--;
        ftree.cY--;
        renderFiletree();
        return;
      }
      ftree.cY--;
    }

    void curTreeDown() {
      if (ftree.cY >= ftree.tree.size() - 1) return;
      if (ftree.cY >= firstShownFile + textAreaLength()) {
        firstShownFile++;
        ftree.cY++;
        renderFiletree();
        return;
      }
      ftree.cY++;
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
        if (cY == firstShownLine + textAreaLength()) {
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
      else if (cX == 0 && config.curWrap && cY != 0) {
        if (cY == firstShownLine) {
          scrollUp();
        } else {
          printf("\033[1A");
        }
        cY--;
        cX = maxPosOfLine(cY);
        int padLeft = (ftree.isShown()) ? ftree.treeWidth + 1 : 0;
        printf("\033[%dG", cX + marginLeft + padLeft);
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
      else if (curIsAtMaxPos() && config.curWrap && cY < lines.size() - 1) {
        if (cY == firstShownLine + winRows - 2 - marginTop) {
          scrollDown();
        } else {
          printf("\033[1E");
        }
        cY++;
        cX = 0;
        int padLeft = (ftree.isShown()) ? ftree.treeWidth + 1 : 0;
        printf("\033[%dG", marginLeft + padLeft);
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

    void gotoBegOfNext() {
      if (curIsAtMaxPos() && cY == lines.size() - 1) return;

      string sc = ";:(){}[]\"";
      int x = cX;
      for (int i = 1; i < lines[cY].length() - cX; i++) {
        char c = lines[cY][cX + i];
        if (sc.find(c) != string::npos && sc.find(lines[cY][cX]) == string::npos) {
          cX += i;
          break;
        }
        else if (c == ' ') {
          cX += i + 1;
          break;
        }
        else if (sc.find(c) == string::npos && sc.find(lines[cY][cX]) != string::npos) {
          cX += i;
          break;
        }
      }
      if (lines[cY][cX] == ' ') {
        gotoBegOfNext();
        return;
      }
      if (cX == x) {
        cY++;
        cX = minPosOfLineIWS(cY);
      }
      syncCurPosOnScr();
    }

    void gotoEndOfNext() {
      if (curIsAtMaxPos() && cY == lines.size() - 1) return;

      string sc = ";:(){}[]\"";
      int x = cX;
      for (int i = 1; i < lines[cY].length() - cX; i++) {
        if (cX + i >= lines[cY].size()) {
          return;
        }
        char cn = lines[cY][cX + i + 1];
        if (cn == ' ') {
          cX += i;
          break;
        }
        else if (sc.find(cn) != string::npos && sc.find(lines[cY][cX]) == string::npos || cX + i == lines[cY].size() - 1) {
          cX += i;
          break;
        }
        else if (sc.find(cn) == string::npos && sc.find(lines[cY][cX]) != string::npos || cX + i == lines[cY].size() - 1) {
          cX += i;
          break;
        }
      }
      if (lines[cY][cX] == ' ') {
        gotoEndOfNext();
        return;
      }
      if (cX == x) {
        cY++;
        cX = minPosOfLineIWS(cY);
        gotoEndOfNext();
        return;
      }
      syncCurPosOnScr();
    }

    void gotoBegOfLast() {
      if (cY == 0 && cX == 0) return;

      string sc = ";:(){}[]\"";
      int x = cX;
      for (int i = 1; i <= cX; i++) {
        char cl = lines[cY][cX - i];
        if ((cl == ' ' || sc.find(cl) != string::npos) && i > 1) {
          cX -= i - 1;
          break;
        }
      }
      if (cX < minPosOfLineIWS(cY) && cY != 0) {
        cY--;
        cX = maxPosOfLine(cY);
      }
      else if (cX == x && cY == 0) {
        cX = minPosOfLineIWS(cY);
      }

      syncCurPosOnScr();
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

    void search() {
      resetMatchSearch();
      searchStr = queryUser("/");
      if (searchStr == "") {
        comLineText = "";
        syncCurPosOnScr();
        return;
      }
      if (searchForMatches() > 0) {
        gotoNextMatch();
        highlightMatches();
      }
      else {
        showErrorMsg("No match found: " + searchStr);
        syncCurPosOnScr();
      }
    }

    string getStrOnCur() {
      string sChars = " :;,.{}[]()";
      char charOnCur = lines[cY][cX];
      if (sChars.find(charOnCur) != string::npos) {
        return string(1, charOnCur);
      }
      int xS = cX;
      int len = 1;
      while (xS > 0 && sChars.find(lines[cY][xS - 1]) == string::npos) {
          xS--;
      }
      while (abs(len+xS) <= lines[cY].length() && sChars.find(lines[cY][xS + len]) == string::npos) {
          len++;
      }
      return lines[cY].substr(xS, len);
    }

    void searchStrOnCur() {
      resetMatchSearch();
      searchStr = getStrOnCur();
      if (searchStr == " ") {
        searchStr = "";
        return;
      }
      if (searchForMatches() > 0) {
        curMatch = getMatchClosestToCur();
        gotoNextMatch();
        highlightMatches();
      }
    }

    int searchForMatches() {
      for (int yp = 0; yp < lines.size(); yp++) {
        int xp = lines[yp].find(searchStr);
        while (xp != std::string::npos) {
          matches.push_back({xp, yp});
          xp = lines[yp].find(searchStr, xp + 1);
        }
      }
      showMsg("<\"" + searchStr +"\"> " + to_string(matches.size()) + " matches");
      syncCurPosOnScr();
      return matches.size();
    }

    void highlightMatches() {
      if (matches.empty()) return;
      cout << "\033[s";
      for (int i = 0; i < matches.size(); i++) {
        if (matches[i].y < firstShownLine) continue;
        else if (matches[i].y > firstShownLine + textAreaLength()) break;
        else {
          printf("\033[%d;%dH", matches[i].y - firstShownLine + marginTop, matches[i].x + marginLeft);
          cout << "\033[7m" << searchStr << "\033[0m";
        }
      }
      cout << "\033[u" << flush;
    }

    int getMatchClosestToCur() {
      int closest = -1;
      int d;
      for (int i = 0; i < matches.size(); i++) {
        d = abs(matches[i].y - cY);
        if (closest < 0 || d < abs(matches[closest].y - cY)) {
          closest = i;
        }
      }
      return closest;
    }

    void gotoNextMatch() {
      if (matches.empty()) return;
      if ( curMatch > 0 && curMatch >= matches.size() - 1) {
        curMatch = 0;
      } 
      else if (curMatch < 0) {
        curMatch = getMatchClosestToCur();
      } else {
        curMatch++;
      }
      gotoMatch();
    }

    void gotoLastMatch() {
      if (matches.empty()) return;
      if (curMatch <= 0) {
        curMatch = matches.size() - 1;
      } else {
        curMatch--;
      }
      gotoMatch();
    }

    void gotoMatch() {
      pos pos = matches[curMatch];
      cX = pos.x;
      cY = pos.y;
      if (cY < firstShownLine || cY > firstShownLine + textAreaLength()) {
        goToLine(cY);
      }
      highlightMatches();
      syncCurPosOnScr();
    }

    void resetMatchSearch() {
      searchStr = "";
      matches.clear();
      curMatch = -1;
      renderShownText(firstShownLine);
    }

    bool matchesHighlighted() {
      if (matches.empty()) return false;
      return true;
    }

    int replaceMatch(const string newStr, pos& p) {
      lines[p.y].replace(p.x, searchStr.length(), newStr);
      unsaved = true;
      return newStr.length() - searchStr.length();
    }

    void replaceAllMatches(string newStr) {
      if (matches.empty()) return;
      int posMoved = 0;
      for (int i = 0; i < matches.size(); i++) {
        if (i > 0 && matches[i-1].y == matches[i].y) {
          matches[i].x += posMoved;
        } else {
          posMoved = 0;
        }
        posMoved += replaceMatch(newStr, matches[i]);
      }
      showMsg("<\"" + searchStr +"\"> -> <\"" + newStr + "\"> " + to_string(matches.size()) + " replaced");
      searchStr = newStr;
      renderShownText(firstShownLine);
      gotoNextMatch();
      highlightMatches();
      syncCurPosOnScr();
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

    void execBashCommand() {
      updateStatBar(true);
      updateCommandBar();

      FILE* fpipe;
      char c = 0;
      string msg = "";
      while (true) {
        string com = queryUser(comLineText);
        if (com == "") return;

        fpipe = (FILE*)popen(com.c_str(), "r");
        if (fpipe == 0) {
          showErrorMsg(com + " failed");
        } else {
          while (fread(&c, sizeof(c), 1, fpipe)) {
            msg += c;
          }
          showMsg(msg);
        }
      }
      int status = pclose(fpipe);
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

      if (comLineText.substr(0,2) == ":/") {
        searchStr = comLineText.substr(2);
        if (searchForMatches() > 0) {
          gotoNextMatch();
          highlightMatches();
        }
        handleEvent(Event::BACK);
        return true;
      }
      if (comLineText.substr(0,3) == ":r/") {
        string str = comLineText.substr(3);
        int posDash = str.find_last_of("/");
        if (posDash == string::npos) {
          handleEvent(Event::BACK);
          return false;
        }
        searchStr = str.substr(0, posDash);
        if (searchForMatches() > 0) {
          gotoNextMatch();
          highlightMatches();
          replaceAllMatches(str.substr(posDash + 1));
        }
        handleEvent(Event::BACK);
        return true;
      }

      if (comLineText.substr(0,3) == ":r ") {
        if (searchStr != "" && matches.size() > 0) {
          replaceAllMatches(comLineText.substr(3));
        }
        handleEvent(Event::BACK);
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
      if (!config.comline_visible) updateStatBar(true);
      comLineText = msg;
      updateCommandBar();
      if (!config.comline_visible) sleep(1);
    }

    void showErrorMsg(string error) {
      if (!config.comline_visible) updateStatBar(true);
      comLineText = "\033[40;31m" + error + "\033[0m";
      updateCommandBar();
      if (!config.comline_visible) sleep(1);
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
      if (curInFileTree) {
        printf("\033[%d;%dH", ftree.cY - firstShownFile + marginTop, ftree.cX);
        fflush(stdout);
      } else {
        int padLeft = (ftree.isShown()) ? ftree.treeWidth + 1 : 0;
        printf("\033[%d;%dH", cY - firstShownLine + marginTop, cX + marginLeft + padLeft);
        fflush(stdout);
      }
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
      if (lines[y].empty() || lines[y].find_first_not_of(' ') == string::npos) return 0;
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
      if (config.lineNum) {
        int padLeft = (ftree.isShown()) ? ftree.treeWidth + 1 : 0;
        string strLNum = to_string(lNum);
        if (config.relativenumber) {
          if (lNum == 0) {
            checkLineNumSpace(to_string(cY+1));
            return "\033[" + to_string(padLeft) + "G\033[97m" + alignR(to_string(cY+1), lineNumPad) + "\033[0m"; // White highlight
          }
          return "\033[" + to_string(padLeft) + "G\033[90m" + alignR(strLNum, lineNumPad) + "\033[0m";
        }
        checkLineNumSpace(strLNum);
        if (lNum == cY + 1) {
          return "\033[" + to_string(padLeft) + "G\033[97m" + alignR(strLNum, lineNumPad) + "\033[0m"; // White highlight
        }
        return "\033[" + to_string(padLeft) + "G\033[90m" + alignR(strLNum, lineNumPad) + "\033[0m";
      }
      return "\0";
    }

    void updateLineNums(int startLine) {
      if (config.lineNum) {
        cout << "\033[s";
        printf("\033[%d;0H", marginTop + startLine - firstShownLine);
        if (config.relativenumber) {
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
        int padLeft = (ftree.isShown()) ? ftree.treeWidth + 1 : 0;
        for (int i = 0; i <= textAreaLength() - lines.size(); i++) {
          printf("\033[40;34m\033[%dG\033[0K%s\033[1E", padLeft, alignR("~", lineNumPad).c_str());
        }
        cout << "\033[0m";
      }
    }

    void fgColor(uint32_t color) {
      printf("\033[38;2;%d;%d;%dm", config.r(color), config.g(color), config.b(color));
    }
    
    void bgColor(uint32_t color) {
      printf("\033[48;2;%d;%d;%dm", config.r(color), config.g(color), config.b(color));
    }

    void printLine(int i) {
      int padLeft = (ftree.isShown()) ? ftree.treeWidth + 1 : 0;
      printf("\033[%dG", marginLeft + padLeft);
      if (lines[i].length() > winCols - marginLeft) {
        cout << lines[i].substr(0, winCols - marginLeft) << "\033[1E";
      }
      else {
        cout << lines[i] << "\033[1E";
      }
      cout << "\033[0m";
    }

    void clearLine() {
      if (ftree.isShown()) {
        printf("\033[%dG\033[0K", ftree.treeWidth + 1);
      } else {
        cout << "\033[2K";
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
        clearLine();
        if (!config.relativenumber) cout << showLineNum(i + 1);
        printLine(i);
      }
      if (count == lines.size()) fillEmptyLines();
      
      cout << "\033[u" << flush;
      if (config.relativenumber) updateLineNums(firstShownLine);
    }

    void renderShownText(int startLine) {
      if (!fileOpen) return;
      cout << "\033[s";
      printf("\033[%d;1H", marginTop);
      for(int i = startLine; i < lines.size() && i <= textAreaLength() + startLine; i++) {
        clearLine();
        printLine(i);
      }
      fillEmptyLines();
      cout << "\033[u" << flush;
      updateLineNums(startLine);
    }

    void renderFiletree() {
      if (!ftree.isShown()) {
        curInFileTree = false;
        renderShownText(firstShownLine);
        syncCurPosOnScr();
        return;
      }

      curInFileTree = true;

      printf("\033[%d;0H", marginTop);
      for (int i = firstShownFile; i < ftree.tree.size() && i <= textAreaLength() + firstShownFile; i++) {
        printf("\033[%dG\033[1K\033[0G", ftree.treeWidth);
        cout << fileTreeLine(i) << "\033[1E";
      }
      fflush(stdout);
      fillEmptyTreeLines();
      renderShownText(firstShownLine);
      syncCurPosOnScr();
    }

    void fillEmptyTreeLines() {
      if (ftree.tree.size() < textAreaLength()) {
        for (int i = 0; i <= textAreaLength() - ftree.tree.size(); i++) {
          printf("\033[%dG\033[1K\033[0G", ftree.treeWidth);
          cout << string(ftree.treeWidth - 1, ' ') << "|";
          cout << "\033[1E";
        }
        fflush(stdout);
      }
    }

    string fileTreeLine(int i) {
      string ret = "";
      int nLen = ftree.tree[i].name.length();
      string fname = ftree.tree[i].name;
      if (nLen > ftree.treeWidth - 3) {
        fname = fname.substr(0, ftree.treeWidth - 3);
      }

      if (ftree.tree[i].isDir) ret += "D \033[34m";
      else ret += "F ";

      if (i == ftree.cY) {
          ret += "\033[7m" + fname + "\033[0m";
      } else {
          ret += fname;
      }

      int spaces = ftree.treeWidth - nLen - 3;
      if (ftree.tree[i].isDir) {
        ret += "\033[0m/";
        spaces--;
    }

      if (spaces > 0) {
        ret += string(spaces, ' ');
      }
      ret += "|";
      return ret;
    }

    void updateFTreeHighlight(int curL, int lastL) {
      printf("\033[%d;%dH\033[1K\033[0G", marginTop + curL - firstShownFile, ftree.treeWidth);
      cout << fileTreeLine(curL) << "\033[1E";
      printf("\033[%d;%dH\033[1K\033[0G", marginTop + lastL - firstShownFile, ftree.treeWidth);
      cout << fileTreeLine(lastL) << "\033[1E";
      fflush(stdout);
    }

    void updateStatBar(bool showCommandLine = false) {
      cout << "\033[s"; // Save cursor pos
      printf("\033[%d;0H\033[2K\r", winRows - 1);
      if (currentState == State::COMMAND || config.comline_visible) {
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
      cout << comLineText;
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

      Clip clip;
      int startX = selectedText.bX;
      int endX = selectedText.eX;
      for (int i = selectedText.bY; i <= selectedText.eY; i++) {
        int len = lines[i].length() - startX;
        if (i == selectedText.eY) {
          len = (endX == lines[i].length()) ? lines[i].length() - startX : endX - startX + 1;
        }
        clip.push_back(lines[i].substr(startX, len));
        startX = 0;
      }
      if (clip.size() == 1 && clip.front().length() == lines[selectedText.bY].length() 
          && selectedText.eX == lines[selectedText.bY].length()) {
        clip.lineTrue();
      }
      clipboard.copyClip(clip);
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
      Clip clip(true);
      clip.push_back(lines[cY]);
      clipboard.push_back(clip);
    }

    void cpLineEnd() {
      Clip clip;
      clip.push_back(lines[cY].substr(cX));
      clipboard.push_back(clip);
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

    void pasteClipboard(int i = -1) {
      if (clipboard.empty()) return;
      if (i > 0 && i >= clipboard.size()) return;
      unsaved = true;
      int begY = cY;

      string remain = "";
      Clip* clip = (i < 0) ? &clipboard[clipboard.size()-1] : &clipboard[i];
      for (string line : *clip) {
        if (lines[cY].empty()) {
          lines[cY] = line;
          cX = (line == "") ? 0 : maxPosOfLine(cY);
        }
        else if (clip->isLine()) {
          lines.insert(lines.begin() + cY + 1, line);
          cY++;
          cX = minPosOfLineIWS(cY);
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

    fstream* openFile(string fullPath, bool createIfNotFound) {
      fstream* file = new fstream(fullPath);

      if(file->is_open()){
        showMsg(fullPath);
      }
      else if (createIfNotFound) {
        ofstream* file = new ofstream(fullPath);
        showMsg(fullPath + " created");
        file->open(fullPath, ios::in);
      }

      return file;
    }

    void readFile(string fName, bool create = false) {
      fileName = fName;
      fstream* fptr = openFile(fullpath(), create);
      readToLines(fptr);
      delete fptr;
      fileOpen = true;
      getWinSize();
      renderShownText(firstShownLine);
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
      ofstream file(fullpath());
      for(const string& line : lines) {
        file << line << '\n';
      }
      unsaved = false;
      string clt = comLineText;
      showMsg("\"" + fileName + "\" " + to_string(lines.size()) + "L, " + to_string(file.tellp()) + "B written");
      comLineText = clt;
      file.close();
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
    fileName = "";
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
