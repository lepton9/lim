#ifndef LIMEDITOR_H
#define LIMEDITOR_H

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <algorithm>
#include <regex>
#include <sys/ioctl.h>
#include "../include/stev.h"
#include "../include/ModeState.h"
#include "../include/Clipboard.h"
#include "../include/Clip.h"
#include "../include/Config.h"
#include "../include/Filetree.h"

using namespace std;

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


class LimEditor : public ModeState {
  public:
    LimEditor();

    void modeNormal();
    void modeInput();
    void modeCommand();
    void modeVisual();
    void start(string fName);
    bool exitFlag();

  private:
    // Member function pointer type
    typedef void (LimEditor::*functionP)(string);
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
    int firstShownCol = 0;
    int firstShownFile = 0;
    vector<string> lines;
    bool unsaved;
    bool exitf;

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
      func("rename", &LimEditor::rename), 
      func("restart", &LimEditor::restart), 
      func("set", &LimEditor::set), 
      func("setconfig", &LimEditor::setconfig),
      func("configpath", &LimEditor::showConfigPath),
      func("path", &LimEditor::showPath),
    };

    string path;
    string fileName;
    string fullpath();

    Config config;

    bool readConfig();
    void clearAll();
    void updateVariables();
    void setconfig(string path);
    void showConfigPath(string arg);
    void rename(string fName);
    void set(string var);
    void restart(string p);
    void showPath(string args);
    void confirmRename(string newName);
    void inputChar(char c);
    string queryUser(string query);
    void newlineEscSeq();
    void tabKey();
    void backspaceKey();
    void deleteKey();
    void comModeDelChar();
    int readKey();
    void printStartUpScreen();
    void fTreeSelect();
    void curMoveFileTree(int c);
    void refreshFileTree();
    void createFile();
    void closeCurFile();
    void removeDirOnCur();
    void removeFileOnCur();
    void renameFileOnCur();
    void copyFileOnCur();
    void pasteFileInCurDir();
    void curTreeUp();
    void curTreeDown();
    void curMove(int c);
    void curUp();
    void curDown();
    void curLeft();
    void curRight();
    void fitTextHorizontal();
    bool is_integer(const string &s);
    unsigned charTOunsigned(const char * c);
    int charToInt(const char * c);
    void gotoBegOfNext();
    void gotoEndOfNext();
    void gotoBegOfLast();
    void goToFileBegin();
    void goToFileEnd();
    void goToLine(int line);
    void search();
    string getStrOnCur();
    void searchStrOnCur();
    int searchForMatches();
    void highlightMatches();
    int getMatchClosestToCur();
    void gotoNextMatch();
    void gotoLastMatch();
    void gotoMatch();
    void resetMatchSearch();
    bool matchesHighlighted();
    int replaceMatch(const string newStr, pos& p);
    void replaceAllMatches(string newStr);
    bool checkForFunctions(string func);
    bool execBashCommand(string com = "");
    bool execCommand();
    int textAreaLength();
    int textAreaWidth();
    void showMsg(string msg);
    void showErrorMsg(string error);
    string getPerrorString(const string& errorMsg);
    void clsResetCur();
    void getCurPosOnScr(int* x, int* y);
    void syncCurPosOnScr();
    bool curIsAtMaxPos();
    bool curIsOutOfScreenHor();
    void setCurToScreenHor();
    int maxPosOfLine(int y);
    int minPosOfLineIWS(int y);
    void scrollUp();
    void scrollDown();
    void scrollLeft();
    void scrollRight();
    string alignR(string s, int w);
    void checkLineNumSpace(string strLNum);
    string showLineNum(int lNum);
    void updateLineNums(int startLine);
    void fillEmptyLines();
    void fgColor(uint32_t color);
    void bgColor(uint32_t color);
    void printLine(int i);
    void clearLine();
    void updateRenderedLines(int startLine, int count = -1);
    void renderShownText(int startLine);
    void renderFiletree();
    void fillEmptyTreeLines();
    string fileTreeLine(int i);
    void showInfoText(vector<string> textLines, int height);
    void updateFTreeHighlight(int curL, int lastL);
    void updateStatBar(bool showCommandLine = false);
    void updateCommandBar();
    void updateSelectedText();
    void updateShowSelection();
    void clearSelectionUpdate();
    void copySelection();
    void deleteSelection();
    void checkSelectionPoints(textArea* selection);
    void cpLine();
    void cpLineEnd();
    void delCpLine();
    void delCpLineEnd();
    void pasteClipboard(int i = 0);
    void getWinSize();
    fstream* openFile(string fullPath, bool createIfNotFound);
    void readFile(string fName, bool create = false);
    void readToLines(fstream *file);
    void overwriteFile();
    void handleExit(bool force);
    void exitLim();
};

#endif
