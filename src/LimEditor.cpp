#include "../include/LimEditor.h"
#include "../include/utils.h"
#include <cassert>
#include <cctype>
#include <string>
#include <vector>

using namespace std;


LimEditor::LimEditor() {
  unsaved = false;
  cur = {0, 0};
  getWinSize();
  readConfig();
}

void LimEditor::modeNormal() {
  // updateStatBar();

  if (fileOpen) {
    setCursorInbounds();
    syncCurPosOnScr();
  }

  int c;
  while(currentState == State::NORMAL) {
    updateStatBar();
    c = readKey();
    if (curInFileTree) {
      keyFiletree(c);
      continue;
    }
    //printf("%d ('%c')\n", c, c);
    switch (c) {
      case ESC_KEY:
        if (matchesHighlighted()) {
          resetMatchSearch();
        }
        handleEvent(Event::BACK);
        break;
      case TAB_KEY:
      case ENTER_KEY:
        break;
      case 'i':
        handleEvent(Event::INPUT);
        break;
      case 'a':
        handleEvent(Event::INPUT);
        if (!curIsAtMaxPos()) curRight();
        break;
      case 'o': {
        int insertInd = cur.y + 1;
        lines.insert(lines.begin() + insertInd, "");
        cur = {0, insertInd};
        syncCurPosOnScr();
        updateRenderedLines(firstShownLine);
        unsaved = true;
        handleEvent(Event::INPUT);
        break;
      }
      case 'O': {
        lines.insert(lines.begin() + cur.y, "");
        cur.x = 0;
        syncCurPosOnScr();
        updateRenderedLines(firstShownLine);
        unsaved = true;
        handleEvent(Event::INPUT);
        break;
      }
      case 'r':
        c = readKey();
        lines[cur.y][cur.x] = c;
        updateRenderedLines(cur.y, 1);
        break;
      case 'I':
        handleEvent(Event::INPUT);
        cur.x = minPosOfLineIWS(cur.y);
        syncCurPosOnScr();
        break;
      case 'A':
        handleEvent(Event::INPUT);
        cur.x = maxPosOfLine(cur.y);
        syncCurPosOnScr();
        break;
      case ':':
        handleEvent(Event::COMMAND);
        break;
      case '!':
        handleEvent(Event::COMMAND);
        comLineText = ":!";
        execBashCommand();
        handleEvent(Event::BACK);
        break;
      case '/':
      case CTRL_F_KEY: // C+f
        handleEvent(Event::COMMAND);
        search();
        handleEvent(Event::BACK);
        break;
      case '*':
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
      case 'c': {
        c = readKey();
        if (c == 'w') {
          textArea area = getStrAreaOnCur();
          selectedText.bY = cur.y;
          selectedText.bX = cur.x;
          selectedText.eY = area.eY;
          selectedText.eX = area.eX;
          deleteSelection();
          syncCurPosOnScr();
          handleEvent(Event::INPUT);
        }
        else if (c == 'i') {
          if (readKey() == 'w') {
            textArea area = getStrAreaOnCur();
            deleteTextArea(&area);
            unsaved = true;
            updateRenderedLines(cur.y, 1);
            syncCurPosOnScr();
            handleEvent(Event::INPUT);
          }
        }
        break;
      }
      case 'p':
        pasteClipboard();
        break;
      case 'W': // Beginning of next word
        gotoBegOfNextOuter();
        break;
      case 'w':
        gotoBegOfNextInner();
        break;
      case 'E': // End of current word or next if at end of word
        gotoEndOfNextOuter();
        break;
      case 'e':
        gotoEndOfNextInner();
        break;
      case 'B': // Beginning of last word
        gotoBegOfLastOuter();
        break;
      case 'b':
        gotoBegOfLastInner();
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
      case 'x':
        delCpChar();
        break;
      case 'd': {
        c = readKey();
        if (c == 'w') {
          textArea area = getStrAreaOnCur();
          selectedText.bY = cur.y;
          selectedText.bX = cur.x;
          selectedText.eY = area.eY;
          selectedText.eX = area.eX;
          copySelection();
          deleteSelection();
          syncCurPosOnScr();
        }
        else if (c == 'i') {
          if (readKey() == 'w') {
            selectedText = getStrAreaOnCur();
            copySelection();
            deleteSelection();
            syncCurPosOnScr();
          }
        }
        else if (c == 'd') {
          delCpLine();
        }
        break;
      }
      case 'D':
        delCpLineEnd();
        break;
      case 'y': {
        c = readKey();
        if (c == 'w') {
          textArea area = getStrAreaOnCur();
          selectedText.bY = cur.y;
          selectedText.bX = cur.x;
          selectedText.eY = area.eY;
          selectedText.eX = area.eX;
          copySelection();
          selectedText.clear();
          syncCurPosOnScr();
        }
        else if (c == 'i') {
          if (readKey() == 'w') {
            selectedText = getStrAreaOnCur();
            copySelection();
            clearSelectionUpdate();
          }
        }
        else if (c == 'y') {
          cpLine();
        }
        break;
      }
      case 'Y':
        cpLineEnd();
        break;
      case 'f':
        c = readKey();
        findCharRight(c);
        break;
      case 'F':
        c = readKey();
        findCharLeft(c);
        break;
      case 't':
        c = readKey();
        findCharRightBefore(c);
        break;
      case 'T':
        c = readKey();
        findCharLeftAfter(c);
        break;
      case 'J':
        joinLines(1);
        break;
      case '<': {
        char ch = readKey();
        if (ch == '<') {
          shiftLeft(1, 0);
        } else {
          int line_count = -1;
          int direction = 0;
          if (isdigit(ch)) {
             line_count = ch - '0';
            ch = readKey();
            while (isdigit(ch)) {
              line_count = concat_unsigned(line_count, ch - '0');
              ch = readKey();
            }
          }
          if (ch == 'j') {
            if (line_count < 0) line_count = 2;
            direction = 1;
          }
          else if (ch == 'k') {
            if (line_count < 0) line_count = 2;
            direction = -1;
          }
          shiftLeft((line_count < 0) ? 1 : line_count, direction);
        }
        break;
      }
      case '>': {
        char ch = readKey();
        if (ch == '>') {
          shiftRight(1, 0);
        } else {
          int line_count = -1;
          int direction = 0;
          if (isdigit(ch)) {
            line_count = ch - '0';
            ch = readKey();
            while (isdigit(ch)) {
              line_count = concat_unsigned(line_count, ch - '0');
              ch = readKey();
            }
          }
          if (ch == 'j') {
            if (line_count < 0) line_count = 2;
            direction = 1;
          }
          else if (ch == 'k') {
            if (line_count < 0) line_count = 2;
            direction = -1;
          }
          shiftRight((line_count < 0) ? 1 : line_count, direction);
        }
        break;
      }
      case CTRL_N_KEY: // C-n
        ftree.toggleShow();
        curInFileTree = true;
        renderFiletree();
        break;
      case CTRL_H_KEY: // C-h
        if (ftree.isShown() && !curInFileTree) {
          curInFileTree = true;
          syncCurPosOnScr();
        }
        break;
      case CTRL_L_KEY: // C-l
        if (ftree.isShown() && curInFileTree) {
          curInFileTree = false;
          syncCurPosOnScr();
        }
        break;
      case CTRL_P_KEY: { // C-p + #, paste from clipboard at index #
        vector<string> lines;
        string nl = "\033[90m\\n\033[0m";
        for (int i = 0; i < clipboard.size(); i++) {
          lines.push_back(
              "\033[34m" + to_string(i) + "\033[0m" + ": " +
              ((clipboard[i].front().isFullLine()) ? nl : "") +
              clipboard[i].front().text().substr(0, textAreaWidth() - 10) +
              ((clipboard[i].front().isFullLine()) ? nl : "") +
              ((clipboard[i].size() > 1)
                   ? (clipboard[i].size() == 2 &&
                      clipboard[i].back().text() == "")
                         ? nl
                         : " [" + to_string(clipboard[i].size()) + "L]"
                   : ""));
        }
        showInfoText(lines, lines.size());
        c = readKey();
        renderFiletree();
        renderShownText(firstShownLine);
        syncCurPosOnScr();
        if (isdigit(c)) {
          int d = c - 48; // 48 => 0
          pasteClipboard((d < 0) ? clipboard.size() - 1 : d);
        }
        break;
      }

      case CTRL_D_KEY:
        cur.y = min(cur.y + scroll_amount, (int)lines.size() - 1);
        if (cur.x > lines[cur.y].length()) cur.x = max((int)lines[cur.y].length() - 1, 0);
        scrollDown(scroll_amount);
        syncCurPosOnScr();
        break;
      case CTRL_U_KEY:
        cur.y = max(cur.y - scroll_amount, 0);
        if (cur.x > lines[cur.y].length()) cur.x = max((int)lines[cur.y].length() - 1, 0);
        scrollUp(scroll_amount);
        syncCurPosOnScr();
        break;

      //Movement
      case 'h': case 'j': case 'k': case 'l':
      case LEFT_KEY: case DOWN_KEY: case UP_KEY: case RIGHT_KEY:
        curMove(c);
        break;
      default: {
        if (isdigit(c)) {
          unsigned d = c - '0';
          int key = readKey();
          while (isdigit(key)) {
            d = concat_unsigned(d, key - '0');
            key = readKey();
          }
          switch (key) {
            case 'h': case 'j': case 'k': case 'l':
            case LEFT_KEY: case DOWN_KEY: case UP_KEY: case RIGHT_KEY:
              curMove(key, d);
              break;
            case 'J':
              joinLines(d);
              break;
          }
        }
        break;
      }
    }
  }
}

void LimEditor::modeInput() {
  getWinSize();
  int c;
  while (currentState == State::INPUT) {
    updateStatBar();
    c = readKey();
    switch (c) {
      case ESC_KEY:
        handleEvent(Event::BACK);
        break;
      case ENTER_KEY:
        unsaved = true;
        newlineEscSeq();
        break;
      case TAB_KEY:
        unsaved = true;
        tabKey();
        break;
      case BACKSPACE_KEY:
        unsaved = true;
        backspaceKey();
        break;
      case DEL_KEY:
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

void LimEditor::modeCommand() {
  int c;
  int comInd = oldCommands.size();
  string curCom = "";
  if (!keepComlineText) {
    comLineText = ":";
  } else {
    keepComlineText = false;
  }
  updateStatBar();

  while (currentState == State::COMMAND) {
    updateCommandBar();
    c = readKey();

    switch (c) {
      case ESC_KEY:
        handleEvent(Event::BACK);
        if (!selectedText.isNull()) {
          clearSelectionUpdate();
        }
        syncCurPosOnScr();
        break;
      case BACKSPACE_KEY:
        comModeDelChar();
        break;
      case ENTER_KEY: {
        if (execCommand()) {
          oldCommands.push_back(comLineText);
        }
        comLineText = "";
        handleEvent(Event::BACK);
        syncCurPosOnScr();
        break;
      }
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

void LimEditor::modeVisual() {
  if (currentState == State::VISUAL) {
    selectedText.bX = cur.x;
    selectedText.bY = cur.y;
    selectedText.eX = cur.x;
    selectedText.eY = cur.y;
  }
  else if (currentState == State::VLINE) {
    selectedText.bX = 0;
    selectedText.bY = cur.y;
    selectedText.eX = lines[cur.y].length();
    selectedText.eY = cur.y;
  }

  getWinSize();
  updateStatBar();
  int c;
  while (currentState == State::VISUAL || currentState == State::VLINE) {
    updateShowSelection();
    c = readKey();
    switch (c) {
      case ESC_KEY:
        clearSelectionUpdate();
        handleEvent(Event::BACK);
        break;
      case ENTER_KEY:
        break;
      case BACKSPACE_KEY:
        break;
      case DEL_KEY:
        deleteSelection();
        handleEvent(Event::BACK);
        break;
      case '*':
        searchStrOnSelection();
        handleEvent(Event::BACK);
        break;
        // TODO: commands on selected text
      case ':': {
        comLineText = ":" + selection_indicator;
        keepComlineText = true;
        handleEvent(Event::COMMAND);
        break;
      }
      case 'y':
        copySelection();
        clearSelectionUpdate();
        handleEvent(Event::BACK);
        break;
      case 'd':
      case 'x':
        copySelection();
        deleteSelection();
        handleEvent(Event::BACK);
        break;
      case 'V':
        handleEvent(Event::VLINE);
        selectionToLineSel(&selectedText);
        break;
      case 'D':
        handleEvent(Event::VLINE);
        selectionToLineSel(&selectedText);
        copySelection();
        deleteSelection();
        handleEvent(Event::BACK);
        break;
      case 'c':
        replaceSelectionEmpty();
        handleEvent(Event::INPUT);
        break;
      case 'p':
        if (!clipboard.empty()) {
          deleteSelection();
          cur.x--;
          pasteClipboard();
        }
        handleEvent(Event::BACK);
        break;
      case 'r':
        c = readKey();
        replaceTextAreaChar(&selectedText, c);
        clearSelectionUpdate();
        handleEvent(Event::BACK);
        break;

      case 'W': // Beginning of next word
      case 'w':
        gotoBegOfNextInner();
        updateSelectedText();
        break;
      case 'E': // End of current word or next if at end of word
      case 'e':
        gotoEndOfNextInner();
        updateSelectedText();
        break;
      case 'B': // Beginning of last word
      case 'b':
        gotoBegOfLastInner();
        updateSelectedText();
        break;
      case 'g':
        c = readKey();
        if (c == 'g') {
          goToFileBegin();
          updateSelectedText();
        }
        break;
      case 'G':
        goToFileEnd();
        updateSelectedText();
        break;
      case '<':
        shiftLeft(1, 0);
        handleEvent(Event::BACK);
        break;
      case '>':
        shiftRight(1, 0);
        handleEvent(Event::BACK);
        break;

      case CTRL_D_KEY:
        cur.y = min(cur.y + scroll_amount, (int)lines.size() - 1);
        scrollDown(scroll_amount);
        syncCurPosOnScr();
        updateSelectedText();
        break;
      case CTRL_U_KEY:
        cur.y = max(cur.y - scroll_amount, 0);
        scrollUp(scroll_amount);
        syncCurPosOnScr();
        updateSelectedText();
        break;

      //Movement
      case 'h': case 'j': case 'k': case 'l':
      case LEFT_KEY: case DOWN_KEY: case UP_KEY: case RIGHT_KEY:
        curMove(c);
        updateSelectedText();
        break;

      default: {
        if (isdigit(c)) {
          unsigned d = c - '0';
          int key = readKey();
          while (isdigit(key)) {
            d = concat_unsigned(d, key - '0');
            key = readKey();
          }
          switch (key) {
            case 'h': case 'j': case 'k': case 'l':
            case LEFT_KEY: case DOWN_KEY: case UP_KEY: case RIGHT_KEY:
              curMove(key, d);
              updateSelectedText();
              updateRenderedLines(firstShownLine);
              break;
            case '<':
              shiftLeft(d, 0);
              handleEvent(Event::BACK);
            case '>':
              shiftRight(d, 0);
              handleEvent(Event::BACK);
          }
        }
        break;
      }
    }
  }
}

void LimEditor::keyFiletree(int c) {
  if (!curInFileTree) return;
  switch (c) {
    case 'h': case 'j': case 'k': case 'l':
    case LEFT_KEY: case DOWN_KEY: case UP_KEY: case RIGHT_KEY:
      curMoveFileTree(c);
    case ESC_KEY:
      handleEvent(Event::BACK);
      break;
    case TAB_KEY:
    case ENTER_KEY: {
      bool isFile = fTreeSelect();
      curInFileTree = (c == TAB_KEY) || !isFile;
      syncCurPosOnScr();
      renderShownText(firstShownLine);
      break;
    }
    case 'a':
      createFile();
      break;
    case 'r':
      renameFileOnCur();
      break;
    case ':':
      handleEvent(Event::COMMAND);
      break;
    case '!':
      handleEvent(Event::COMMAND);
      comLineText = ":!";
      execBashCommand();
      handleEvent(Event::BACK);
      break;
    case '/':
    case CTRL_F_KEY: // C+f
      handleEvent(Event::COMMAND);
      search();
      handleEvent(Event::BACK);
      break;
    case 'c':
      copyFileOnCur();
      break;
    case 'p':
      pasteFileInCurDir();
      break;
    case 'd':
      removeFileOnCur();
      break;
    case CTRL_N_KEY: // C-n
      ftree.toggleShow();
      curInFileTree = false;
      renderFiletree();
      break;
    case CTRL_H_KEY: // C-h
      if (ftree.isShown() && !curInFileTree) {
        curInFileTree = true;
        syncCurPosOnScr();
      }
      break;
    case CTRL_L_KEY: // C-l
      if (ftree.isShown() && curInFileTree) {
        curInFileTree = false;
        syncCurPosOnScr();
      }
      break;
  }
}

string LimEditor::fullpath() {
  return path + "/" + fileName;
}

void LimEditor::start(string fName) {
  exitf = false;
  path = filesystem::current_path();
  ftree = Filetree(path);
  if (fName == "") {
    curInFileTree = true;
    ftree.toggleShow();
    renderFiletree();
    showMsg(":lim");
    printStartUpScreen();
  } else {
    readFile(fName, true);
  }
}

bool LimEditor::readConfig() {
  config.parse();
  updateVariables();
  return true;
}

void LimEditor::clearAll() {
  path = "";
  fileName = "";
  searchStr = "";
  comLineText = "";
  matches.clear();
  fileOpen = false;
  firstShownLine = 0;
  firstShownCol = 0;
  firstShownFile = 0;
  curMatch = -1;
  lines.clear();
  unsaved = false;
  clipboard.clear();
  oldCommands.clear();
  selectedText.clear();
}

void LimEditor::updateVariables() {
  if (config.lineNum) marginLeft = 7;
  else marginLeft = 2;
}

void LimEditor::setconfig(string path) {
  string newPath;
  bool success = false;
  if (path == "") {
    newPath = queryUser("Set path to \".limconfig\": ");
    if (newPath == "") return;
  }
  else {
    newPath = path;
  }
  success = config.setFilePath(newPath);
  if (success) {
    showMsg("Path set to " + newPath, true);
    updateVariables();
    renderShownText(firstShownLine);
  } else {
    showErrorMsg("Not a valid path " + newPath);
  }
}

void LimEditor::showConfigPath(string arg) {
  showMsg(config.getFilePath(), true);
  syncCurPosOnScr();
}

void LimEditor::rename(string fName) {
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

void LimEditor::set(string var) {
  if (var == "") {
    return;
  }
  bool success = config.set(var);
  if (success) {
    updateVariables();
    renderShownText(firstShownLine);
    showMsg("Updated " + var, true);
  }
  else {
    showErrorMsg("Unknown option: " + var);
  }
}

void LimEditor::restart(string p) {
  getWinSize();
  clsResetCur();
  readConfig();
  firstShownLine = 0;
  firstShownCol = 0;
  firstShownFile = 0;
  if (ftree.isShown()) renderFiletree();
  renderShownText(firstShownLine);
  syncCurPosOnScr();
}

void LimEditor::confirmRename(string newName) {
  string ans = queryUser("Rename the file \"" + newName + "\"? [y/n]: ");
  if (ans == "y") {
    int result = std::rename(fileName.c_str(), newName.c_str());
    if (result == 0) {
      fileName = newName;
      showMsg("File renamed to \"" + newName + "\"", true);
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

void LimEditor::showPath(string args) {
  showMsg(fullpath(), true);
  syncCurPosOnScr();
}

void LimEditor::help(string arg) {
  vector<string> helpText;
  for (func f : functions) {
    char line[128];
    sprintf(line, ":%-20s | %s", f.name.c_str(), f.info.c_str());
    helpText.push_back(string(line));
  }
  helpText.push_back("");
  helpText.push_back("Searching and replacing:");
  char line[128];
  sprintf(line, ":%-20s | %s", "/\'string\'", "search for matches (C-f)");
  helpText.push_back(string(line));
  sprintf(line, ":%-20s | %s", "r/'s1'/'s2'", "replace s1 with s2");
  helpText.push_back(string(line));
  sprintf(line, ":%-20s | %s", "r 'string'", "replace all found matches");
  helpText.push_back(string(line));
  showInfoText(helpText, helpText.size());
  char c = readKey();
  renderFiletree();
  renderShownText(firstShownLine);
  syncCurPosOnScr();
}

string LimEditor::queryUser(string query) {
  comLineText = query;
  updateStatBar(true);
  updateCommandBar();

  string ans = "";
  while(1) {
    char c = readKey();
    if (c == ESC_KEY) {
      comLineText.clear();
      return "";
    }
    else if (c == ENTER_KEY) {
      return ans;
    }
    else if (c == BACKSPACE_KEY && ans.length() > 0) {
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

void LimEditor::inputChar(char c) {
  lines[cur.y].insert(cur.x, 1, c);
  cur.x++;
  if (cur.x > textAreaWidth() + firstShownCol) {
    scrollRight();
  } else {
    cout << "\033[1C" << flush;
    updateRenderedLines(cur.y, 1);
  }
}

void LimEditor::newlineEscSeq() {
  int padLeft = (ftree.isShown()) ? ftree.treeWidth + 1 : 0;
  string newLine = lines[cur.y].substr(cur.x);
  lines[cur.y].erase(cur.x);
  cur.y++;
  lines.insert(lines.begin() + cur.y, newLine);
  cur.x = 0;
  if (cur.y-1 == firstShownLine + winRows - 2 - marginTop) {
    scrollDown();
    printf("\033[%dG", marginLeft + padLeft);
  } else {
    printf("\033[1E\033[%dG", marginLeft + padLeft);
  }
  fflush(stdout);
  updateRenderedLines(cur.y-1);
}

void LimEditor::tabKey() {
  lines[cur.y].insert(cur.x, string(config.indentAm, ' '));
  cur.x += config.indentAm;
  printf("\033[%dC", config.indentAm);
  fflush(stdout);
  updateRenderedLines(cur.y, 1);
}

void LimEditor::backspaceKey() {
  int padLeft = (ftree.isShown()) ? ftree.treeWidth + 1 : 0;
  int ePos;
  if (cur.x == 0) {
    if (cur.y == 0) return;
    string delLine = lines[cur.y];
    lines.erase(lines.begin() + cur.y, lines.begin() + cur.y + 1);
    cur.y--;
    cur.x = lines[cur.y].length();
    lines[cur.y].append(delLine);
    printf("\033[1A\033[%dG", cur.x + marginLeft + padLeft);
    updateRenderedLines(cur.y);
  } else {
    lines[cur.y].erase(cur.x - 1, 1);
    cur.x--;
    printf("\033[1D");
    updateRenderedLines(cur.y, 1);
  }
  fflush(stdout);
}

void LimEditor::deleteKey() {
  if (cur.x == lines[cur.y].length()) {
    if (cur.y == lines.size()-1) return;
    string delLine = lines[cur.y+1];
    lines.erase(lines.begin() + cur.y + 1);
    lines[cur.y] += delLine;
    updateRenderedLines(cur.y);
  } else {
    lines[cur.y].erase(cur.x, 1);
    updateRenderedLines(cur.y, 1);
  }
}

void LimEditor::comModeDelChar() {
  if (!comLineText.empty()) {
    comLineText.pop_back();
  }
}

int LimEditor::readKey() {
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

void LimEditor::printStartUpScreen() {
  cout << "\033[s";

  vector<string> logo = {
    "  _     _           ",
    " | |   (_)_ __ ___  ",
    " | |   | | '_ ` _ \\ ",
    " | |___| | | | | | |",
    " |_____|_|_| |_| |_|"
  };

  printf("\033[%d;0H", marginTop);
  for (vector<string>::iterator it = logo.begin(); it < logo.end(); it++) {
    printf("\033[%dG", ftree.treeWidth + 5);
    printf("%s\033[1E", it->c_str());
  }
  cout << "\033[u";
}

// Returns true if a file is selected
bool LimEditor::fTreeSelect() {
  if (!curInFileTree) return false;

  if (ftree.getElementOnCur()->isDir) {
    firstShownFile = 0;
    showMsg(ftree.getElementOnCur()->path);
    ftree.changeDirectory(ftree.getElementOnCur()->path);
    renderFiletree();
    syncCurPosOnScr();
    return false;
  } else {
    if (ftree.getElementOnCur()->name == fileName) {
      return true;
    }
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
        else if (ans == "" && comLineText.empty()) return false;
      }
    }
    path = ftree.getElementOnCur()->path.parent_path();
    readFile(ftree.getElementOnCur()->name);
    firstShownLine = 0;
    cur.x = 0, cur.y = 0;
    return true;
  }
}

void LimEditor::curMoveFileTree(int c) {
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

void LimEditor::refreshFileTree() {
  ftree.refresh();
  renderFiletree();
  syncCurPosOnScr();
}

void LimEditor::createFile() {
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

void LimEditor::closeCurFile() {
  clearAll();
  curInFileTree = true;
  if (!ftree.isShown()) ftree.toggleShow();
  clsResetCur();
  renderFiletree();
  printStartUpScreen();
}

void LimEditor::removeDirOnCur() {
  string dPathRem = ftree.getElementOnCur()->path;
  string ans = queryUser("Delete directory and its contents \"" + dPathRem + "\"? (y/n): ");

  if (ans == "y") {
    int c = filesystem::remove_all(dPathRem);
    if (dPathRem == path) {
      closeCurFile();
    }
    refreshFileTree();
    showMsg(to_string(c) + " files or directories deleted", true);
    syncCurPosOnScr();
  }
  else {
    comLineText = "";
    updateCommandBar();
  }
}

void LimEditor::removeFileOnCur() {
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
    showMsg("File " + fPathRem + " was removed", true);
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

void LimEditor::renameFileOnCur() {
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
  showMsg(oldPath + " -> " + newPath, true);
  syncCurPosOnScr();
}

void LimEditor::copyFileOnCur() {
  if (!curInFileTree) return;
  ftree.copy();
  if (ftree.copied->isDir) {
    showMsg("Directory " + ftree.copied->path.string() + " copied recursively", true);
  } else {
    showMsg("File " + ftree.copied->path.string() + " copied", true);
  }
  syncCurPosOnScr();
}

void LimEditor::pasteFileInCurDir() {
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
  showMsg(oldPath + " copied to " + newPath, true);
  syncCurPosOnScr();
}

void LimEditor::curTreeUp() {
  if (ftree.cY <= 0) return;
  if (ftree.cY == firstShownFile && firstShownFile > 0) {
    firstShownFile--;
    ftree.cY--;
    renderFiletree();
    return;
  }
  ftree.cY--;
}

void LimEditor::curTreeDown() {
  if (ftree.cY >= ftree.tree.size() - 1) return;
  if (ftree.cY >= firstShownFile + textAreaLength()) {
    firstShownFile++;
    ftree.cY++;
    renderFiletree();
    return;
  }
  ftree.cY++;
}

void LimEditor::curMove(int c, int n) {
  for (int i = 0; i < n; i++) {
    if (c == 'h' || c == LEFT_KEY) {
      curLeft();
    } else if (c == 'j' || c == DOWN_KEY) {
      curDown();
    } else if (c == 'k' || c == UP_KEY) {
      curUp();
    } else if (c == 'l' || c == RIGHT_KEY) {
      curRight();
    }
    else if (c == 'H') {
      gotoBegOfLastInner();
    } else if (c == 'J') {
      if (firstShownLine + textAreaLength() < lines.size() - 1) {
        cur.y++;
        scrollDown();
      }
    } else if (c == 'K') {
      if (firstShownLine > 0) {
        cur.y--;
        scrollUp();
      }
    } else if (c == 'L') {
      gotoBegOfNextInner();
    }
  }
  updateStatBar();
  updateLineNums(firstShownLine);
}

void LimEditor::curUp() {
  if (lines.empty()) {
    return;
  }
  if (cur.y > 0) {
    if (cur.y == firstShownLine) {
      scrollUp();
    } else {
      cout << "\033[1A" << flush;
    }
    cur.y--;

    if (curIsAtMaxPos()) {
      cur.x = maxPosOfLine(cur.y);
      fitTextHorizontal();
      syncCurPosOnScr();
    }
  }
  else {
    cur.x = 0;
    if (firstShownCol != 0) {
      firstShownCol = 0;
      renderShownText(firstShownLine);
    }
    syncCurPosOnScr();
  }
}

void LimEditor::curDown() {
  if (lines.empty()) {
    return;
  }
  if (cur.y < lines.size() - 1) {
    if (cur.y == firstShownLine + textAreaLength()) {
      scrollDown();
    } else {
      cout << "\033[1B" << flush;
    }
    cur.y++;

    if (curIsAtMaxPos()) {
      cur.x = maxPosOfLine(cur.y);
      fitTextHorizontal();
      syncCurPosOnScr();
    }
  }
  else {
    cur.x = maxPosOfLine(cur.y);
    fitTextHorizontal();
    syncCurPosOnScr();
  }
}

void LimEditor::curLeft() {
  if (lines.empty()) {
    return;
  }
  if (cur.x > 0) {
    cur.x--;
    if (cur.x < firstShownCol && firstShownCol > 0) {
      scrollLeft();
    } else {
      cout << "\033[1D" << flush;
    }
  }
  else if (cur.x == 0 && config.curWrap && cur.y != 0) {
    if (cur.y == firstShownLine) {
      scrollUp();
    } else {
      printf("\033[1A");
    }
    cur.y--;
    cur.x = maxPosOfLine(cur.y);
    fitTextHorizontal();
  }
}

void LimEditor::curRight() {
  if (lines.empty()) {
    return;
  }
  int padLeft = (ftree.isShown()) ? ftree.treeWidth + 1 : 0;
  if (!curIsAtMaxPos()) {
    cur.x++;
    if (cur.x - firstShownCol >= textAreaWidth() && lines[cur.y].length() - 1 > cur.x) {
      scrollRight();
    } else {
      cout << "\033[1C" << flush;
    }
  }
  else if (curIsAtMaxPos() && config.curWrap && cur.y < lines.size() - 1) {
    if (cur.y == firstShownLine + winRows - 2 - marginTop) {
      scrollDown();
    } else {
      printf("\033[1E");
    }
    cur.y++;
    cur.x = 0;
    if (firstShownCol != 0) {
      firstShownCol = 0;
      renderShownText(firstShownLine);
    }
    printf("\033[%dG", marginLeft + padLeft);
  }
}

// direction < 0: UP, direction > 0: DOWN, direction == 0: one line
void LimEditor::shiftLeft(int line_count, int direction) {
  if (!selectedText.isNull()) {
    checkSelectionPoints(&selectedText);
    int count = line_count;
    line_count = selectedText.eY - selectedText.bY + 1;
    for (int i = selectedText.bY; i < selectedText.bY + line_count; i++) {
      removeCharFromBeg(&lines[i], ' ', count * config.indentAm);
    }
    clearSelectionUpdate();
  } else {
    int startLine = cur.y;
    if (direction < 0) {
      startLine = std::max(0, cur.y - (line_count - 1));
      line_count = cur.y - startLine + 1;
    }
    for (int i = startLine; i < startLine + line_count && i < lines.size(); i++) {
      removeCharFromBeg(&lines[i], ' ', config.indentAm);
    }
    updateRenderedLines(startLine, line_count);
  }
  unsaved = true;
}

// direction < 0: UP, direction > 0: DOWN, direction == 0: one line
void LimEditor::shiftRight(int line_count = 1, int direction = 0) {
  if (!selectedText.isNull()) {
    checkSelectionPoints(&selectedText);
    int count = line_count;
    line_count = selectedText.eY - selectedText.bY + 1;
    for (int i = selectedText.bY; i < selectedText.bY + line_count; i++) {
      lines[i].insert(0, string(count * config.indentAm, ' '));
    }
    clearSelectionUpdate();
  } else {
    int startLine = cur.y;
    if (direction < 0) {
      startLine = std::max(0, cur.y - (line_count - 1));
      line_count = cur.y - startLine + 1;
    }
    for (int i = startLine; i < startLine + line_count && i < lines.size(); i++) {
      lines[i].insert(0, string(config.indentAm, ' '));
    }
    updateRenderedLines(startLine, line_count);
  }
  unsaved = true;
}

void LimEditor::joinLines(int n) {
  for (int i = 0; i < n; i++) {
    if (cur.y == lines.size() - 1) continue;
    string joinedLine = lines[cur.y + 1];
    lines.erase(lines.begin() + cur.y + 1);
    trim_beg(joinedLine);
    if (!joinedLine.empty()) {
      string curLine = lines[cur.y];
      cur.x = curLine.length();
      if (curLine.empty()) {
        lines[cur.y].append(joinedLine);
      }
      lines[cur.y].append(" " + joinedLine);
    }
    unsaved = true;
  }
  updateRenderedLines(cur.y);
  syncCurPosOnScr();
}

void LimEditor::findCharRight(char c) {
  int pos = lines[cur.y].find(c, cur.x + 1);
  if (pos != string::npos) {
    printf("\033[%dC", pos - cur.x);
    cur.x = pos;
  }
}

void LimEditor::findCharLeft(char c) {
  if (cur.x == 0) return;
  int pos = lines[cur.y].rfind(c, cur.x - 1);
  if (pos != string::npos) {
    printf("\033[%dD", cur.x - pos);
    cur.x = pos;
  }
}

void LimEditor::findCharRightBefore(char c) {
  int pos = lines[cur.y].find(c, cur.x + 1);
  int moveAmount = pos - cur.x - 1;
  if (pos != string::npos && moveAmount > 0) {
    printf("\033[%dC", moveAmount);
    cur.x = pos - 1;
  }
}

void LimEditor::findCharLeftAfter(char c) {
  if (cur.x == 0) return;
  int pos = lines[cur.y].rfind(c, cur.x - 1);
  int moveAmount = cur.x - pos - 1;
  if (pos != string::npos && moveAmount > 0) {
    printf("\033[%dD", moveAmount);
    cur.x = pos + 1;
  }
}

void LimEditor::fitTextHorizontal() {
  int padLeft = (ftree.isShown()) ? ftree.treeWidth + 1 : 0;
  if (cur.x > winCols - marginLeft - padLeft) {
    firstShownCol = cur.x - winCols + marginLeft + padLeft + 1;
    renderShownText(firstShownLine);
    printf("\033[%dG", winCols - 1);
  } else {
    if (firstShownCol != 0) {
      firstShownCol = 0;
      renderShownText(firstShownLine);
    }
    printf("\033[%dG", cur.x + marginLeft + padLeft);
  }
}

void LimEditor::gotoBegOfNextInner() {
  if (curIsAtMaxPos() && cur.y == lines.size() - 1) return;
  int x = cur.x;
  for (int i = 1; i < lines[cur.y].length() - cur.x; i++) {
    char c = lines[cur.y][cur.x + i];
    if (s_chars.find(c) != string::npos && s_chars.find(lines[cur.y][cur.x]) == string::npos) {
      cur.x += i;
      break;
    } else if (c == ' ') {
      cur.x += i + 1;
      break;
    } else if (s_chars.find(c) == string::npos && s_chars.find(lines[cur.y][cur.x]) != string::npos) {
      cur.x += i;
      break;
    }
  }
  if (lines[cur.y][cur.x] == ' ') {
    gotoBegOfNextInner();
    return;
  } if (cur.x == x && cur.y < lines.size() - 1) {
    cur.y++;
    cur.x = minPosOfLineIWS(cur.y);
  } else if (cur.x == x) {
    cur.x = maxPosOfLine(cur.y);
  }
  syncCurPosOnScr();
}

void LimEditor::gotoBegOfNextOuter() {
  if (curIsAtMaxPos() && cur.y == lines.size() - 1) return;
  int x = cur.x;
  for (int i = 1; i < lines[cur.y].length() - cur.x; i++) {
    char c = lines[cur.y][cur.x + i];
    if (c == ' ') {
      cur.x += i + 1;
      break;
    }
  }
  if (lines[cur.y][cur.x] == ' ') {
    gotoBegOfNextOuter();
    return;
  }
  if (cur.x == x && cur.y < lines.size() - 1) {
    cur.y++;
    cur.x = minPosOfLineIWS(cur.y);
  }
  else if (cur.x == x) {
    cur.x = maxPosOfLine(cur.y);
  }
  syncCurPosOnScr();
}

void LimEditor::gotoEndOfNextInner() {
  if (curIsAtMaxPos() && cur.y == lines.size() - 1) return;
  int x = cur.x;
  for (int i = 1; i < lines[cur.y].length() - cur.x; i++) {
    char cn = lines[cur.y][cur.x + i + 1];
    if (s_chars.find(cn) != string::npos && s_chars.find(lines[cur.y][cur.x]) == string::npos || cur.x + i == lines[cur.y].size() - 1) {
      cur.x += i;
      break;
    }
    else if (s_chars.find(cn) == string::npos && s_chars.find(lines[cur.y][cur.x]) != string::npos || cur.x + i == lines[cur.y].size() - 1) {
      cur.x += i;
      break;
    }
  }
  if (lines[cur.y][cur.x] == ' ') {
    gotoEndOfNextInner();
    return;
  }
  if (cur.x == x) {
    cur.y++;
    cur.x = minPosOfLineIWS(cur.y);
    gotoEndOfNextInner();
    return;
  }
  syncCurPosOnScr();
}

void LimEditor::gotoEndOfNextOuter() {
  if (curIsAtMaxPos() && cur.y == lines.size() - 1) return;
  int x = cur.x;
  for (int i = 1; i < lines[cur.y].length() - cur.x; i++) {
    if (cur.x + i + 1 == lines[cur.y].size()) {
      cur.x += i;
      break;
    }
    char cn = lines[cur.y][cur.x + i + 1];
    if (cn == ' ') {
      cur.x += i;
      break;
    }
  }
  if (lines[cur.y][cur.x] == ' ') {
    gotoEndOfNextOuter();
    return;
  }
  if (cur.x == x) {
    cur.y++;
    cur.x = minPosOfLineIWS(cur.y);
    gotoEndOfNextOuter();
    return;
  }
  syncCurPosOnScr();
}

void LimEditor::gotoBegOfLastInner() {
  if (cur.y == 0 && cur.x == 0) return;
  if (cur.x == 0) {
    cur.y--;
    cur.x = maxPosOfLine(cur.y);
    gotoBegOfLastInner();
    return;
  }
  char cl;
  int x = cur.x;
  for (int i = 1; i <= cur.x; i++) {
    cl = lines[cur.y][cur.x - i];
    if ((cl == ' ' || s_chars.find(cl) != string::npos) && i > 1) {
      cur.x -= (i - 1);
      break;
    }
  }
  if (((cur.x <= minPosOfLineIWS(cur.y) && (lines[cur.y][cur.x] == ' ' || lines[cur.y].empty()))) && cur.y != 0) {
    cur.y--;
    cur.x = maxPosOfLine(cur.y);
    gotoBegOfLastInner();
    return;
  }
  else if (cur.x == x) {
    cur.x = minPosOfLineIWS(cur.y);
  }
  syncCurPosOnScr();
}

void LimEditor::gotoBegOfLastOuter() {
  if (cur.y == 0 && cur.x == 0) return;
  if (cur.x == 0) {
    cur.y--;
    cur.x = maxPosOfLine(cur.y);
    gotoBegOfLastOuter();
    return;
  }
  int x = cur.x;
  for (int i = 1; i <= cur.x; i++) {
    char cl = lines[cur.y][cur.x - i];
    if (cl == ' ' && i > 1) {
      cur.x -= (i - 1);
      break;
    }
  }
  if (((cur.x <= minPosOfLineIWS(cur.y) && (lines[cur.y][cur.x] == ' ' || lines[cur.y].empty()))) && cur.y != 0) {
    cur.y--;
    cur.x = maxPosOfLine(cur.y);
    gotoBegOfLastOuter();
    return;
  }
  else if (cur.x == x) {
    cur.x = minPosOfLineIWS(cur.y);
  }
  syncCurPosOnScr();
}

void LimEditor::goToFileBegin() {
  cur.y = 0;
  if (curIsAtMaxPos()) {
    cur.x = maxPosOfLine(cur.y);
  }
  firstShownLine = 0;
  renderShownText(firstShownLine);
  syncCurPosOnScr();
}

void LimEditor::goToFileEnd() {
  cur.y = lines.size() - 1;
  if (curIsAtMaxPos()) {
    cur.x = maxPosOfLine(cur.y);
  }
  firstShownLine = (cur.y < textAreaLength()) ? 0 : cur.y - textAreaLength();
  renderShownText(firstShownLine);
  syncCurPosOnScr();
}

void LimEditor::goToLine(int line) {
  if (line < 0) {
    line = 0;
  }
  else if (line >= lines.size()) {
    line = lines.size() - 1;
  }
  cur.y = line;
  if (curIsAtMaxPos()) {
    cur.x = maxPosOfLine(cur.y);
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

void LimEditor::search() {
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

string LimEditor::textAreaToString(textArea* area) {
  string text;
  if (area->bY == area->eY) {
    text.append(lines[area->bY].substr(area->bX, area->eX - area->bX + 1));
    return text;
  }
  for (int y = area->bY; y <= area->eY; y++) {
    if (y == area->bY) {
      text.append(lines[y].substr(area->bX, lines[y].length() - area->bX) + '\n');
    } else if (y == area->eY) {
      text.append(lines[y].substr(0, area->eX + 1));
    } else {
      text.append(lines[y] + '\n');
    }
  }
  if (area->bY != area->eY) {
    text = "\"" + text + "\"";
  }
  return text;
}

vector<string> LimEditor::textAreaToVector(textArea* area) {
  vector<string> text;
  if (area->bY == area->eY) {
    text.push_back(lines[area->bY].substr(area->bX, area->eX - area->bX + 1));
    return text;
  }
  for (int y = area->bY; y <= area->eY; y++) {
    if (y == area->bY) {
      text.push_back(lines[y].substr(area->bX, lines[y].length() - area->bX));
    } else if (y == area->eY) {
      text.push_back(lines[y].substr(0, area->eX + 1));
    } else {
      text.push_back(lines[y]);
    }
  }
  return text;
}

textArea LimEditor::getStrAreaOnCur() {
  textArea word;
  string line = lines[cur.y];
  if (s_chars.find(line[cur.x]) != string::npos) {
    word.bX = cur.x;
    word.eX = cur.x;
    word.bY = cur.y;
    word.eY = cur.y;
    return word;
  }
  int iL = cur.x;
  int iR = cur.x;
  while (word.bX < 0 || word.eX < 0) {
    if (word.bX < 0 && (iL == 0 || s_chars.find(line[iL - 1]) != string::npos)) {
      word.bX = iL;
      word.bY = cur.y;
    } else {
      iL--;
    }
    if (word.eX < 0 && (iR == line.length() - 1 || s_chars.find(line[iR + 1]) != string::npos)) {
      word.eX = iR;
      word.eY = cur.y;
    } else {
      iR++;
    }
  }
  return word;
}

string LimEditor::getStrOnCur() {
  textArea area = getStrAreaOnCur();
  vector<string> text = textAreaToVector(&area);
  if (!text.empty()) {
    return text.front();
  }
  return "";
}

void LimEditor::searchStrOnSelection() {
  if (selectedText.isNull()) return;
  resetMatchSearch();
  vector<string> text = textAreaToVector(&selectedText);
  selectedText.clear();
  searchStr = text.front();
  if (trim_c(searchStr).empty()) {
    searchStr = "";
    return;
  }
  int matches = searchForMatches();
  if (matches > 0) {
    curMatch = getMatchClosestToCur();
    gotoNextMatch();
    highlightMatches();
  }
}

void LimEditor::searchStrOnCur() {
  resetMatchSearch();
  searchStr = getStrOnCur();
  if (trim_c(searchStr).empty()) {
    searchStr = "";
    return;
  }
  int matches = searchForMatches();
  if (matches > 0) {
    curMatch = getMatchClosestToCur();
    gotoNextMatch();
    highlightMatches();
  }
}

int LimEditor::searchForMatchesArea(textArea* area) {
  if (!area) {
    return searchForMatches();
  }
  checkSelectionPoints(area);
  for (int y = area->bY; y <= area->eY && y < lines.size(); y++) {
    int xB = (y == area->bY) ? area->bX : 0;
    int xE = (y == area->eY) ? area->eX : lines[y].length();
    int xp = lines[y].find(searchStr, xB);
    while (xp != std::string::npos && xp + searchStr.length() <= xE) {
      matches.push_back({xp, y});
      xp = lines[y].find(searchStr, xp + 1);
    }
  }
  showMsg("<\"" + searchStr +"\"> " + to_string(matches.size()) + " matches");
  syncCurPosOnScr();
  return matches.size();
}

int LimEditor::searchForMatches() {
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

void LimEditor::highlightMatches() {
  if (matches.empty()) return;
  int padLeft = (ftree.isShown()) ? ftree.treeWidth + 1 : 0;
  cout << "\033[s";
  for (int i = 0; i < matches.size(); i++) {
    if (matches[i].y < firstShownLine) continue;
    else if (matches[i].y > firstShownLine + textAreaLength()) break;
    else {
      printf("\033[%d;%dH", matches[i].y - firstShownLine + marginTop, matches[i].x + marginLeft + padLeft);
      cout << "\033[7m" << searchStr << "\033[0m";
    }
  }
  cout << "\033[u" << flush;
}

int LimEditor::getMatchClosestToCur() {
  int closest = -1;
  int d;
  for (int i = 0; i < matches.size(); i++) {
    d = abs(matches[i].y - cur.y);
    if (closest < 0 || d < abs(matches[closest].y - cur.y)) {
      closest = i;
    }
  }
  return closest;
}

void LimEditor::gotoNextMatch() {
  if (matches.empty()) return;
  if (curMatch >= matches.size() - 1) {
    curMatch = 0;
  } 
  else if (curMatch < 0) {
    curMatch = getMatchClosestToCur();
  } else {
    curMatch++;
  }
  gotoMatch();
}

void LimEditor::gotoLastMatch() {
  if (matches.empty()) return;
  if (curMatch <= 0) {
    curMatch = matches.size() - 1;
  } else {
    curMatch--;
  }
  gotoMatch();
}

void LimEditor::gotoMatch() {
  pos pos = matches[curMatch];
  cur.x = pos.x;
  cur.y = pos.y;
  if (cur.y < firstShownLine || cur.y > firstShownLine + textAreaLength()) {
    goToLine(cur.y);
  }
  highlightMatches();
  syncCurPosOnScr();
}

void LimEditor::resetMatchSearch() {
  searchStr = "";
  matches.clear();
  curMatch = -1;
  renderShownText(firstShownLine);
}

bool LimEditor::matchesHighlighted() {
  if (matches.empty()) return false;
  return true;
}

int LimEditor::replaceMatch(const string newStr, pos& p) {
  lines[p.y].replace(p.x, searchStr.length(), newStr);
  unsaved = true;
  return newStr.length() - searchStr.length();
}

void LimEditor::replaceAllMatches(string newStr) {
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

void LimEditor::replaceTextAreaChar(textArea* area, char c) {
  checkSelectionPoints(area);
  for (int y = area->bY; y <= area->eY; y++) {
    int bI = (y == area->bY) ? area->bX : 0;
    int eI = (y == area->eY) ? area->eX : lines[y].length() - 1;
    std::fill(lines[y].begin() + bI, lines[y].begin() + eI + 1, c);
  }
}

bool LimEditor::checkForFunctions(string func) {
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
      return true;
    }
  }
  return false;
}

bool LimEditor::execBashCommand(string com) {
  updateStatBar(true);
  updateCommandBar();

  bool ret = false;
  FILE* fpipe;
  char c = 0;
  string msg = "";
  string clText = comLineText;

  if (com == "") {
    com = queryUser(comLineText);
  }

  if (com != "") {
    fpipe = (FILE*)popen(com.c_str(), "r");
    if (fpipe == 0) {
      showErrorMsg(com + " failed");
      ret = false;
    } else {
      while (fread(&c, sizeof(c), 1, fpipe)) {
        msg += c;
      }
      showMsg(msg);
      ret = true;
      int status = pclose(fpipe);
    }
  }
  readKey(); // Press any key to clean the output
  comLineText = clText;

  // clsResetCur();
  renderFiletree();
  renderShownText(firstShownLine);
  syncCurPosOnScr();
  return ret;
}

bool LimEditor::execCommand() {
  string com = comLineText.substr(1);
  if (is_integer(com)) {
    const char* clt = com.c_str();
    int line = charToInt(clt);
    goToLine(--line);
    updateLineNums(firstShownLine);
    return true;
  }

  if (comLineText.substr(0,2) == ":!") {
    return execBashCommand(comLineText.substr(2));
  }

  if (comLineText.substr(0, 7) == (":" + selection_indicator + "!") &&
      !selectedText.isNull()) {
    string pipeString = textAreaToString(&selectedText);
    selectedText.clear();
    return execBashCommand(comLineText.substr(7) + ' ' + pipeString);
  }

  if (checkForFunctions(com)) {
    comLineText = ':' + com;
    return true;
  }

  if (comLineText.substr(0,2) == ":/" || comLineText.substr(0,3) == ":s/") {
    string str = comLineText.substr(comLineText.find_first_of('/') + 1);
    int indSlash = str.find('/');
    searchStr = (indSlash == string::npos) ? str
                                           : str.substr(0, indSlash);
    if (searchForMatches() > 0) {
      gotoNextMatch();
      highlightMatches();
    }
    handleEvent(Event::BACK);
    return true;
  }

  if (comLineText.substr(0,7) == (":" + selection_indicator + "/")) {
    string str = comLineText.substr(7);
    int indSlash = str.find('/');
    searchStr = (indSlash == string::npos) ? str
                                           : str.substr(0, indSlash);
    int matches = searchForMatchesArea(&selectedText);
    clearSelectionUpdate();
    if (matches > 0) {
      gotoNextMatch();
      highlightMatches();
    }
    handleEvent(Event::BACK);
    return true;
  }

  if (comLineText.substr(0,4) == ":\%s/") {
    string str = comLineText.substr(4);
    int indSlash = str.find('/');
    searchStr = (indSlash == string::npos) ? str
                                           : str.substr(0, indSlash);
    string replaceString = (indSlash == string::npos) ? ""
                                           : str.substr(indSlash + 1);
    if (searchForMatches() > 0) {
      gotoNextMatch();
      highlightMatches();
      replaceAllMatches(replaceString);
    }
    handleEvent(Event::BACK);
    return true;
  }

  if (comLineText.substr(0,8) == (":" + selection_indicator + "s/")) {
    string str = comLineText.substr(8);
    int indSlash = str.find('/');
    searchStr = (indSlash == string::npos) ? str
                                           : str.substr(0, indSlash);
    string replaceString = (indSlash == string::npos) ? ""
                                           : str.substr(indSlash + 1);
    int matches = searchForMatchesArea(&selectedText);
    clearSelectionUpdate();
    if (matches > 0) {
      gotoNextMatch();
      highlightMatches();
      replaceAllMatches(replaceString);
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
        handleExit(true);
      } else {
        handleExit(false);
      }
    }
  }
  if (comLineText.length() > 1) return true;
  return false;
}

int LimEditor::textAreaLength() {
  return winRows - marginTop - 2;
}

int LimEditor::textAreaWidth() {
  int padLeft = (ftree.isShown()) ? ftree.treeWidth + 1 : 0;
  int w = winCols - marginLeft - padLeft;
  return w;
}

void LimEditor::showMsg(string msg, bool force) {
  if (!config.comline_visible) updateStatBar(true);
  comLineText = msg;
  updateCommandBar();
  if (!config.comline_visible && force) {
    sleep(1);
  }
}

void LimEditor::showErrorMsg(string error) {
  if (!config.comline_visible) updateStatBar(true);
  comLineText = "\033[40;31m" + error + "\033[0m";
  updateCommandBar();
  if (!config.comline_visible) sleep(1);
}

string LimEditor::getPerrorString(const string& errorMsg) {
    stringstream ss;
    ss << errorMsg << ": " << strerror(errno);
    return ss.str();
}

void LimEditor::clsResetCur() {
  cur.x = 0;
  cur.y = 0;
  printf("\033[2J\033[%d;%dH", marginTop, marginLeft);
  fflush(stdout);
}

void LimEditor::getCurPosOnScr(int* x, int* y) {
  cout << "\033[6n";
  char response[32];
  int bytesRead = read(STDIN_FILENO, response, sizeof(response) - 1);
  response[bytesRead] = '\0';
  sscanf(response, "\033[%d;%dR", x, y);
}

void LimEditor::syncCurPosOnScr() {
  if (curInFileTree) {
    printf("\033[%d;%dH", ftree.cY - firstShownFile + marginTop, ftree.cX);
    fflush(stdout);
  } else {
    if (curIsOutOfScreenHor()) {
      setCurToScreenHor();
      renderShownText(firstShownLine);
    }
    if (curIsOutOfScreenVer()) {
      setCurToScreenVer();
      renderShownText(firstShownLine);
    }
    int padLeft = (ftree.isShown()) ? ftree.treeWidth + 1 : 0;
    printf("\033[%d;%dH", cur.y - firstShownLine + marginTop, cur.x - firstShownCol + marginLeft + padLeft);
    updateLineNums(firstShownLine);
    fflush(stdout);
  }
}

bool LimEditor::curIsAtMaxPos() {
  if (lines[cur.y].empty()) return true;
  if (currentState == State::NORMAL && cur.x >= lines[cur.y].length() - 1) {
    return true;
  }
  else if (cur.x >= lines[cur.y].length()) {
    return true;
  }
  return false;
}

bool LimEditor::curIsOutOfScreenVer() {
  return cur.y > firstShownLine + textAreaLength() || cur.y < firstShownLine;
}

bool LimEditor::curIsOutOfScreenHor() {
  return cur.x > firstShownCol + textAreaWidth() || cur.x < firstShownCol;
}

void LimEditor::setCurToScreenVer() {
  if (cur.y < firstShownLine) {
    firstShownLine = max(cur.y - textAreaLength() / 2, 0);
  } else if (cur.y > firstShownLine + textAreaLength()) {
    firstShownLine = min(cur.y - textAreaLength() / 2,
                         (int)lines.size() - textAreaLength() - 1);
  }
}

void LimEditor::setCurToScreenHor() {
  if (cur.x < firstShownCol) {
    firstShownCol = cur.x;
  } else {
    firstShownCol = cur.x - textAreaWidth();
  }
}

int LimEditor::maxPosOfLine(int y) {
  if (lines[y].empty()) return 0;
  return (currentState == State::NORMAL) ? lines[y].length() - 1 : lines[y].length();
}

int LimEditor::minPosOfLineIWS(int y) {
  if (lines[y].empty() || lines[y].find_first_not_of(' ') == string::npos) return 0;
  return lines[y].find_first_not_of(' ');
}

void LimEditor::scrollUp(int n) {
  firstShownLine = max(firstShownLine - n, 0);
  renderShownText(firstShownLine);
}

void LimEditor::scrollDown(int n) {
  firstShownLine = min(firstShownLine + n, (int)lines.size() - textAreaLength() - 1);
  if (firstShownLine < 0) firstShownLine = 0;
  renderShownText(firstShownLine);
}

void LimEditor::scrollLeft(int n) {
  firstShownCol = max(firstShownCol - n, 0);
  renderShownText(firstShownLine);
}

void LimEditor::scrollRight(int n) {
  firstShownCol = min(firstShownCol + n, (int)lines[cur.y].length() - textAreaWidth() - 1);
  renderShownText(firstShownLine);
}

void LimEditor::checkLineNumSpace(string strLNum) {
  if (strLNum.length() > lineNumPad) {
    lineNumPad = strLNum.length();
    if (marginLeft - 1 <= lineNumPad) marginLeft++;
  }
}

string LimEditor::showLineNum(int lNum) {
  if (config.lineNum) {
    int padLeft = (ftree.isShown()) ? ftree.treeWidth + 1 : 0;
    string strLNum = to_string(lNum);
    if (config.relativenumber) {
      if (lNum == 0) {
        checkLineNumSpace(to_string(cur.y+1));
        return "\033[" + to_string(padLeft) + "G\033[97m" + alignR(to_string(cur.y+1), lineNumPad) + "\033[0m"; // White highlight
      }
      return "\033[" + to_string(padLeft) + "G\033[90m" + alignR(strLNum, lineNumPad) + "\033[0m";
    }
    checkLineNumSpace(strLNum);
    if (lNum == cur.y + 1) {
      return "\033[" + to_string(padLeft) + "G\033[97m" + alignR(strLNum, lineNumPad) + "\033[0m"; // White highlight
    }
    return "\033[" + to_string(padLeft) + "G\033[90m" + alignR(strLNum, lineNumPad) + "\033[0m";
  }
  return "\0";
}

void LimEditor::updateLineNums(int startLine) {
  if (config.lineNum) {
    cout << "\033[s";
    printf("\033[%d;0H", marginTop + startLine - firstShownLine);
    if (config.relativenumber) {
      for(int i = startLine; i < lines.size() && i <= winRows-marginTop-2+startLine; i++) {
        cout << showLineNum(abs(cur.y-i)) << "\033[1E";
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

void LimEditor::fillEmptyLines() {
  int emptyLines = textAreaLength() - (lines.size() - firstShownLine);
  if (emptyLines > 0) {
    int padLeft = (ftree.isShown()) ? ftree.treeWidth + 1 : 0;
    printf("\033[%d;0H", textAreaLength() - emptyLines + 1);
    for (int i = 0; i <= emptyLines; i++) {
      printf("\033[40;34m\033[%dG\033[0K%s\033[1E", padLeft, alignR("~", lineNumPad).c_str());
    }
    cout << "\033[0m";
  }
}

void LimEditor::fgColor(uint32_t color) {
  printf("\033[38;2;%d;%d;%dm", config.r(color), config.g(color), config.b(color));
}

void LimEditor::bgColor(uint32_t color) {
  printf("\033[48;2;%d;%d;%dm", config.r(color), config.g(color), config.b(color));
}

void LimEditor::printLine(int i) {
  int padLeft = (ftree.isShown()) ? ftree.treeWidth + 1 : 0;
  printf("\033[%dG", marginLeft + padLeft);
  fgColor(config.fg_color);

  if (lines[i].length() > winCols - marginLeft - padLeft) {
    cout << lines[i].substr(firstShownCol, winCols - marginLeft - padLeft) << "\033[1E";
  }
  else {
    if (firstShownCol > lines[i].length()) {
      cout << "\033[1E";
    } else {
      cout << lines[i].substr(firstShownCol, lines[i].length() - firstShownCol) << "\033[1E";
    }
  }
  cout << "\033[0m";
}

void LimEditor::clearLine() {
  if (ftree.isShown()) {
    printf("\033[%dG\033[0K", ftree.treeWidth + 1);
  } else {
    cout << "\033[2K";
  }
}

void LimEditor::updateRenderedLines(int startLine, int count) {
  cout << "\033[s";
  printf("\033[%d;0H", marginTop + startLine - firstShownLine);
  if (count < 0) count = textAreaLength() - (startLine - firstShownLine) + 1;

  int maxIter = min(textAreaLength() - (startLine - firstShownLine)
      , textAreaLength() + 1);
  int maxIterWithOffset = min(maxIter, textAreaLength() - (startLine - firstShownLine));

  for (int i = startLine; i < lines.size() && i <= startLine + maxIterWithOffset 
      && i - startLine < count; i++) {
    clearLine();
    if (!config.relativenumber) cout << showLineNum(i + 1);
    printLine(i);
  }
  fillEmptyLines();
  cout << "\033[u" << flush;
  if (config.relativenumber) updateLineNums(firstShownLine);
}

void LimEditor::renderShownText(int startLine) {
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

void LimEditor::renderFiletree() {
  if (!ftree.isShown()) {
    curInFileTree = false;
    renderShownText(firstShownLine);
    syncCurPosOnScr();
    return;
  }

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

void LimEditor::fillEmptyTreeLines() {
  if (ftree.tree.size() < textAreaLength()) {
    for (int i = 0; i <= textAreaLength() - ftree.tree.size(); i++) {
      printf("\033[%dG\033[1K\033[0G", ftree.treeWidth);
      cout << string(ftree.treeWidth - 1, ' ') << "|";
      cout << "\033[1E";
    }
    fflush(stdout);
  }
}

string LimEditor::fileTreeLine(int i) {
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

void LimEditor::showInfoText(vector<string> textLines, int height) {
  int padLeft = (ftree.isShown()) ? ftree.treeWidth + 1 : 1;
  int maxHeight = textAreaLength();
  string border(textAreaWidth() + marginLeft + (ftree.isShown() ? 1 : 0), '_');
  cout << "\033[s";
  printf("\033[%d;0H", marginTop + textAreaLength() - ((config.comline_visible) ? 1 : 0) - ((height < maxHeight) ? height : maxHeight));
  printf("\033[%dG%s\033[1E", padLeft, border.c_str());
  for (int i = 0; i < textLines.size() && i < maxHeight; i++) {
    printf("\033[%dG", padLeft);
    cout << "\033[0K  " << textLines[i] << "\033[1E";
  }
  printf("\033[%dG%s\033[1E", padLeft, border.c_str());
  cout << "\033[u";
  fflush(stdout);
  // readKey();
}

void LimEditor::updateFTreeHighlight(int curL, int lastL) {
  printf("\033[%d;%dH\033[1K\033[0G", marginTop + curL - firstShownFile, ftree.treeWidth);
  cout << fileTreeLine(curL) << "\033[1E";
  printf("\033[%d;%dH\033[1K\033[0G", marginTop + lastL - firstShownFile, ftree.treeWidth);
  cout << fileTreeLine(lastL) << "\033[1E";
  fflush(stdout);
}

void LimEditor::updateStatBar(bool showCommandLine) {
  cout << "\033[s"; // Save cursor pos
  printf("\033[%d;0H\033[2K\r", winRows - 1);
  if (currentState == State::COMMAND || config.comline_visible) {
    showCommandLine = true;
  }
  if (!showCommandLine) {
    printf("\033[%d;0H", winRows); // Move cursor to the last line
  }
  cout << "\033[2K\r"; // Clear current line
  string mode;
  string modeText;
  if (currentState == State::INPUT) {
    modeText = " INPUT ";
    mode = "\033[30;42m" + modeText + "\033[0m";
  }
  else if (currentState == State::COMMAND) {
    modeText = " COMMAND ";
    mode = "\033[30;43m" + modeText + "\033[0m";
  }
  else if (currentState == State::VISUAL) {
    modeText = " VISUAL ";
    mode = "\033[30;45m" + modeText + "\033[0m";
  }
  else if (currentState == State::VLINE) {
    modeText = " V-LINE ";
    mode = "\033[30;45m" + modeText + "\033[0m";
  }
  else {
    modeText = " NORMAL ";
    mode = "\033[30;44m" + modeText + "\033[0m";
  }
  string saveText = " s ";
  string saveStatus;
  if (unsaved) saveStatus = "\033[41;30m" + saveText + "\033[47;30m";
  else saveStatus = "\033[42;30m" + saveText + "\033[47;30m";
  string eInfo = "| " + fileName + " " + saveStatus;
  string curInfo = to_string(cur.y + 1) + ", " + to_string(cur.x + 1) + " ";
 
  int fillerSpace = winCols - modeText.length() - eInfo.length() - curInfo.length() + (saveStatus.length() - saveText.length());

  if (fillerSpace < 0) {
    eInfo = "| " + saveStatus;
    fillerSpace = winCols - mode.length() - eInfo.length() - curInfo.length() + (saveStatus.length() - saveText.length());
    fillerSpace = (fillerSpace < 0) ? 0 : fillerSpace;
  }

  cout << mode;
  bgColor(config.sb_bg_color);
  fgColor(config.sb_fg_color);
  cout << eInfo;
  cout << string(fillerSpace, ' ') << curInfo;
  cout << "\033[0m\033[u" << flush;
}

void LimEditor::updateCommandBar() {
  printf("\033[%d;1H", winRows); // Move cursor to the last line
  cout << "\033[2K\r"; // Clear current line
  cout << comLineText;
  cout << "\033[0m" << flush;
}

void LimEditor::updateSelectedText() {
  if (currentState == State::VISUAL) {
    selectedText.eX = cur.x;
    selectedText.eY = cur.y;
  }
  else if (currentState == State::VLINE) {
    selectedText.eX = lines[cur.y].length();
    selectedText.eY = cur.y;
  }
}

void LimEditor::updateShowSelection() {
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

  int padLeft = (ftree.isShown()) ? ftree.treeWidth + 1 : 0;

  fgColor(config.fg_color);
  cout << "\033[s\033[7m"; // Inverse color
  printf("\033[%d;%dH", marginTop + sel.bY - firstShownLine, marginLeft + padLeft + ((firstShownCol > sel.bX) ? 0 : sel.bX - firstShownCol));
  if (sel.bY == sel.eY) {
    int hlBeg = (firstShownCol > sel.bX) ? firstShownCol : sel.bX;
    int hlLen = ((sel.eX > firstShownCol + textAreaWidth()) ? firstShownCol + textAreaWidth() : sel.eX + 1) - hlBeg;
    cout << lines[sel.bY].substr(hlBeg, hlLen);
  } else {
    int hlBeg = (firstShownCol > sel.bX) ? firstShownCol : sel.bX;
    int hlLen = ((lines[sel.bY].length() > firstShownCol + textAreaWidth()) ? firstShownCol + textAreaWidth() : lines[sel.bY].length()) - hlBeg;
    if (firstShownCol < lines[sel.bY].length()) {
      cout << lines[sel.bY].substr(hlBeg, hlLen) << "\033[1E";
    } else {
      cout << "\033[1E";
    }
    for (int i = sel.bY + 1; i < sel.eY; i++) {
      printf("\033[%dG", marginLeft + padLeft);
      printLine(i);
      cout << "\033[7m";
    }
    printf("\033[%dG", marginLeft + padLeft);
    if (firstShownCol < sel.eX) {
      int hlLen = (sel.eX > firstShownCol + textAreaWidth()) ? textAreaWidth() : sel.eX - firstShownCol + 1;
      cout << lines[sel.eY].substr(firstShownCol, hlLen);
    }
  }
  cout << "\033[0m\033[u" << flush;
}

void LimEditor::clearSelectionUpdate() {
  cur.y = selectedText.bY;
  cur.x = selectedText.bX;
  checkSelectionPoints(&selectedText);
  syncCurPosOnScr();
  updateRenderedLines(firstShownLine);
  selectedText.clear();
}

void LimEditor::copySelection() {
  if (selectedText.isNull()) return;
  checkSelectionPoints(&selectedText);

  Clip clip;
  int startX = selectedText.bX;
  int endX = selectedText.eX;
  for (int i = selectedText.bY; i <= selectedText.eY; i++) {
    int len = lines[i].length() - startX;
    if (i == selectedText.eY) {
      len = ((endX >= lines[i].length()) ? lines[i].length() : endX + 1) - startX;
    }
    clip.push_back(LineYank(lines[i].substr(startX, len), currentState == State::VLINE));
    startX = 0;
  }
  if (endX >= lines[selectedText.eY].length() && !lines[selectedText.eY].empty() && currentState == State::VISUAL) {
    clip.push_back(LineYank("", false)); // \n
  }
  clipboard.copyClip(clip);
}

void LimEditor::deleteTextArea(textArea* area) {
  checkSelectionPoints(area);
  int startLine = area->bY;
  int endLine = area->eY;
  if (startLine == endLine)  {
    if (lines[startLine].length() == area->eX - area->bX) {
      lines.erase(lines.begin() + startLine);
    } else {
      if (area->eX == lines[startLine].length()) {
        lines[startLine].erase(area->bX);
        lines[startLine].append(lines[startLine + 1]);
        lines.erase(lines.begin() + startLine + 1);
      } else {
        lines[startLine].erase(area->bX, area->eX - area->bX + 1);
      }
    }
  } else {
    int delLines = 0;
    if (area->eX == lines[endLine - delLines].length()) {
      lines.erase(lines.begin() + startLine, lines.begin() + endLine + 1);
    } else {
      lines[startLine].erase(area->bX);
      lines[endLine].erase(0, area->eX + 1);
      lines[startLine].append(lines[endLine]);
      lines.erase(lines.begin() + startLine + 1, lines.begin() + endLine + 1);
    }
  }
  handleFileEmpty();
}

void LimEditor::setCursorInbounds() {
  if (cur.y < 0) cur.y = 0;
  else if (cur.y >= lines.size()) {
    cur.y = lines.size() - 1;
  }
  if (cur.x < 0) cur.x = 0;
  else if (curIsAtMaxPos()) {
    cur.x = maxPosOfLine(cur.y);
  }
}

void LimEditor::handleFileEmpty() {
  if (lines.size() == 0) {
    lines.push_back("");
  }
}

void LimEditor::deleteSelection() {
  if (selectedText.isNull()) return;
  unsaved = true;
  deleteTextArea(&selectedText);
  clearSelectionUpdate();
}

void LimEditor::replaceSelectionEmpty() {
  if (selectedText.isNull()) return;
  unsaved = true;
  deleteTextArea(&selectedText);
  if (currentState == State::VLINE) {
    lines.insert(lines.begin() + selectedText.bY, "");
  }
  clearSelectionUpdate();
}

void LimEditor::swapSelectionVISUAL(textArea* selection) {
  if (selection->eY < selection->bY || 
    (selection->bY == selection->eY && selection->eX < selection->bX)) {
    swap(selection->bY, selection->eY);
    swap(selection->bX, selection->eX);
  }
}

void LimEditor::swapSelectionVLINE(textArea* selection) {
  if (selection->eY < selection->bY) {
    swap(selection->bY, selection->eY);
    selection->bX = 0;
    selection->eX = lines[selection->eY].length();
  }
}

void LimEditor::checkSelectionPoints(textArea* selection) {
  if (currentState == State::VISUAL) {
    swapSelectionVISUAL(selection);
  }
  else if (currentState == State::VLINE) {
    swapSelectionVLINE(selection);
  }
  else {
    if (selection->bX == 0 && selection->eX == lines[selection->eY].length()) {
      swapSelectionVLINE(selection);
    } else {
      swapSelectionVISUAL(selection);
    }
  }
}

void LimEditor::selectionToLineSel(textArea* selection) {
  if (selectedText.isNull()) return;
  checkSelectionPoints(&selectedText);
  selection->bX = 0;
  selection->eX = lines[selection->eY].size();
}

void LimEditor::cpChar() {
  Clip clip;
  clip.push_back(LineYank(lines[cur.y].substr(cur.x, 1), false));
  clipboard.copyClip(clip);
}

void LimEditor::cpLine() {
  Clip clip;
  clip.push_back(LineYank(lines[cur.y], true));
  clipboard.copyClip(clip);
}

void LimEditor::cpLineEnd() {
  Clip clip;
  clip.push_back(LineYank(lines[cur.y].substr(cur.x), false));
  clip.push_back(LineYank("", false));
  clipboard.copyClip(clip);
}

void LimEditor::delCpLine() {
  unsaved = true;
  cpLine();
  int fileLen = lines.size();
  if (fileLen == 1) {
    lines[0].clear();
  } else {
    lines.erase(lines.begin() + cur.y);
  }
  if (cur.y > 0 && cur.y == fileLen - 1) {
    cur.y--;
  }
  updateRenderedLines(cur.y);
  if (curIsAtMaxPos()) {
    cur.x = maxPosOfLine(cur.y);
  }
  syncCurPosOnScr();
}

void LimEditor::delCpLineEnd() {
  unsaved = true;
  cpLineEnd();
  lines[cur.y].erase(cur.x);
  updateRenderedLines(cur.y, 1);
  if (cur.x > 0) {
    cur.x--;
    syncCurPosOnScr();
  }
}

void LimEditor::delCpChar() {
  unsaved = true;
  cpChar();
  lines[cur.y].erase(cur.x, 1);
  updateRenderedLines(cur.y, 1);
  if (curIsAtMaxPos()) {
    cur.x = maxPosOfLine(cur.y);
    syncCurPosOnScr();
  }
}

void LimEditor::pasteClipboard(int i) {
  if (clipboard.empty()) return;
  if (i >= clipboard.size()) return;
  unsaved = true;
  int begY = cur.y;

  string remain = "";
  Clip* clip = (i < 0) ? &clipboard[clipboard.size()-1] : &clipboard[i];
  int l = 0;
  for (LineYank line: *clip) {
    if (line.isFullLine() || l > 0) {
      lines.insert(lines.begin() + cur.y + 1, line.text());
      cur.y++;
      cur.x = minPosOfLineIWS(cur.y);
    }
    else if (lines[cur.y].empty()) {
      lines[cur.y] = line.text();
      cur.x = (line.text() == "") ? 0 : maxPosOfLine(cur.y);
    }
    else {
      remain = lines[cur.y].substr(cur.x + 1);
      lines[cur.y].erase(cur.x + 1);
      lines[cur.y].append(line.text());
      cur.x = maxPosOfLine(cur.y);
    }
    l++;
  }
  lines[cur.y].append(remain);

  if (cur.y - firstShownLine > textAreaLength()) {
    firstShownLine = cur.y - winRows + marginTop + 2;
    renderShownText(firstShownLine);
  }
  else if (cur.y == begY) {
    updateRenderedLines(begY, 1);
  } else {
    updateRenderedLines(begY);
  }
  syncCurPosOnScr();
}

void LimEditor::getWinSize() {
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

fstream* LimEditor::openFile(string fullPath, bool createIfNotFound) {
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

void LimEditor::readFile(string fName, bool create) {
  fileName = fName;
  fstream* fptr = openFile(fullpath(), create);
  readToLines(fptr);
  delete fptr;
  fileOpen = true;
  getWinSize();
  renderShownText(firstShownLine);
  syncCurPosOnScr();
}

void LimEditor::readToLines(fstream *file) {
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

void LimEditor::overwriteFile() {
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

void LimEditor::handleExit(bool force) {
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

  if (ftree.isShown() && !curInFileTree) {
    closeCurFile();
  } else {
    exitLim();
  }
}

bool LimEditor::exitFlag() {
  return exitf;
}

void LimEditor::exitLim() {
  exitf = true;
}

