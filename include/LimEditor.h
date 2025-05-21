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
#include <sys/ioctl.h>
#include "../include/stev.h"
#include "../include/ModeState.h"
#include "../include/Clipboard.h"
#include "../include/Clip.h"
#include "../include/Config.h"
#include "../include/Filetree.h"

using namespace std;

enum Keys {
  ESC_KEY = 27,
  ENTER_KEY = 10,
  TAB_KEY = 9,
  BACKSPACE_KEY = 127,
  CTRL_F_KEY = 6,
  CTRL_N_KEY = 14,
  CTRL_H_KEY = 8,
  CTRL_L_KEY = 12,
  CTRL_P_KEY = 16,
  CTRL_D_KEY = 4,
  CTRL_U_KEY = 21,
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
        string info;
        functionP f;
        func(string n, functionP fp) : name(n), f(fp), info("") {};
        func(string n, string i, functionP fp) : name(n), info(i), f(fp) {};
    };

    const string s_chars = " ~!@#$%^&*=(){}[]|:`'\";+-<>?,.\\/";
    const string selection_indicator = "'<,'>";
    const int scroll_amount = 30;

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

    bool keepComlineText = false;
    string comLineText = "";
    vector<string> oldCommands;
    Clipboard clipboard;
    textArea selectedText;
    pos cur;

    vector<char> validCommands = {'w', 'q', '!'};
    vector<func> functions = {
      func("rename", "rename the current file", &LimEditor::rename),
      func("restart", "restart Lim", &LimEditor::restart),
      func("set", "change config variable", &LimEditor::set),
      func("setconfig", "set path to .limconfig", &LimEditor::setconfig),
      func("configpath", "show current config file path", &LimEditor::showConfigPath),
      func("path", "show path of the current file or directory", &LimEditor::showPath),
      func("help", "help", &LimEditor::help),
    };

    string path;
    string fileName;
    string fullpath();

    Config config;

    void keyFiletree(int c);
    bool readConfig();
    void clearAll();
    void updateVariables();
    void setconfig(string path);
    void showConfigPath(string arg);
    void rename(string fName);
    void set(string var);
    void restart(string p);
    void showPath(string args);
    void help(string arg);
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
    bool fTreeSelect();
    void curMoveFileTree(int c);
    void refreshFileTree();
    void createFile();
    void closeCurFile();
    void curTreeUp();
    void removeDirOnCur();
    void removeFileOnCur();
    void renameFileOnCur();
    void copyFileOnCur();
    void pasteFileInCurDir();
    void curTreeDown();
    void curMove(int c, int n = 1);
    void curUp();
    void curDown();
    void curLeft();
    void curRight();
    void shiftLeft(int lines, int direction);
    void shiftRight(int lines, int direction);
    void joinLines(int n);
    void findCharRight(char c);
    void findCharLeft(char c);
    void findCharRightBefore(char c);
    void findCharLeftAfter(char c);
    void fitTextHorizontal();
    void gotoBegOfNextInner();
    void gotoEndOfNextInner();
    void gotoBegOfLastInner();
    void gotoBegOfNextOuter();
    void gotoEndOfNextOuter();
    void gotoBegOfLastOuter();
    void goToFileBegin();
    void goToFileEnd();
    void goToLine(int line);
    void search();
    string textAreaToString(textArea* area);
    vector<string> textAreaToVector(textArea* area);
    textArea getStrAreaOnCur();
    string getStrOnCur();
    void searchStrOnSelection();
    void searchStrOnCur();
    int searchForMatchesArea(textArea* area = NULL);
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
    void replaceTextAreaChar(textArea* area, char c);
    bool checkForFunctions(string func);
    bool execBashCommand(string com = "");
    bool execCommand();
    int textAreaLength();
    int textAreaWidth();
    void showMsg(string msg, bool force = false);
    void showErrorMsg(string error);
    string getPerrorString(const string& errorMsg);
    void clsResetCur();
    void getCurPosOnScr(int* x, int* y);
    void syncCurPosOnScr();
    bool curIsAtMaxPos();
    bool curIsOutOfScreenVer();
    bool curIsOutOfScreenHor();
    void setCurToScreenVer();
    void setCurToScreenHor();
    void setCursorInbounds();
    void handleFileEmpty();
    int maxPosOfLine(int y);
    int minPosOfLineIWS(int y);
    void scrollUp(int n = 1);
    void scrollDown(int n = 1);
    void scrollLeft(int n = 1);
    void scrollRight(int n = 1);
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
    void deleteTextArea(textArea* area);
    void deleteSelection();
    void replaceSelectionEmpty();
    void swapSelectionVISUAL(textArea* selection);
    void swapSelectionVLINE(textArea* selection);
    void checkSelectionPoints(textArea* selection);
    void selectionToLineSel(textArea* selection);
    void cpChar();
    void cpLine();
    void cpLineEnd();
    void delCpLine();
    void delCpLineEnd();
    void delCpChar();
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
