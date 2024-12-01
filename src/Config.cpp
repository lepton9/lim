#include "../include/Config.h"
#include "../include/utils.h"

bool Config::handleKeyValue(std::string key, std::string value) {
  bool changed = false;
  if (key == "tab_indent") {
    if (isdigit(value.front())) {
      indentAm = stoi(value);
      changed = true;
    }
  }
  else if (key == "cursor_wrap") {
    if (value == "true") {
      curWrap = true;
      changed = true;
    }
    else if (value == "false") {
      curWrap = false;
      changed = true;
    }
  }
  else if (key == "number") {
    if (value == "true") {
      lineNum = true;
      changed = true;
    }
    else if (value == "false") {
      lineNum = false;
      changed = true;
    }
  }
  else if (key == "number_style") {
    if (value == "normal") {
      relativenumber = false;
      changed = true;
    }
    else if (value == "relative") {
      relativenumber = true;
      changed = true;
    }
  }
  else if (key == "comline_visible") {
    if (value == "true") {
      comline_visible = true;
      changed = true;
    }
    else if (value == "false") {
      comline_visible = false;
      changed = true;
    }
  }
  else if (key == "bg_color") {
    if (is_hex(value)) {
      bg_color = std::strtoul(value.c_str(), NULL, 16);
      changed = true;
    }
  }
  else if (key == "fg_color") {
    if (is_hex(value)) {
      fg_color = std::strtoul(value.c_str(), NULL, 16);
      changed = true;
    }
  }
  else if (key == "sb_bg_color") {
    if (is_hex(value)) {
      sb_bg_color = std::strtoul(value.c_str(), NULL, 16);
      changed = true;
    }
  }
  else if (key == "sb_fg_color") {
    if (is_hex(value)) {
      sb_fg_color = std::strtoul(value.c_str(), NULL, 16);
      changed = true;
    }
  }
  return changed;
}

void Config::parse() {
  setDefault();
  findConfig();
  std::fstream* file = openFile();
  if (!file) return;
  if (file->peek() == EOF) return;
  std::string line;
  while (getline(*file, line)) {
    if (strip(line) == "" || line.rfind("//", 0) == 0) {
      continue;
    }
    int equalSignPos = line.find_first_of("=");
    if (equalSignPos == std::string::npos || equalSignPos == line.length() - 1) {
      continue;
    }
    std::string key = line.substr(0, equalSignPos);
    std::string value = line.substr(equalSignPos + 1);
    handleKeyValue(key, value);
  }
  delete file;
}

std::fstream* Config::openFile() {
  std::fstream* file = new std::fstream(filePath + fileName);
  return file;
}

void Config::setDefault() {
  indentAm = 2;
  curWrap = true;
  lineNum = true;
  relativenumber = false;
  comline_visible = false;
  bg_color = 0x000000;
  fg_color = 0xFFFFFF;
  sb_bg_color = 0xFFFFFF;
  sb_fg_color = 0x000000;
}

uint8_t Config::r(uint32_t color) {
  return ((0xFF0000&color)>>8*2);
}
uint8_t Config::g(uint32_t color) {
  return ((0x00FF00&color)>>8*1);
}
uint8_t Config::b(uint32_t color) {
  return ((0x0000FF&color));
}

bool Config::setFilePath(std::string path) {
  bool ret = false;
  int dashPos = path.find_last_of('/');
  if (dashPos == std::string::npos && path == fileName) {
    filePath = "./";
    ret = true;
  }
  if (path.substr(dashPos + 1) == fileName) {
    filePath = path.substr(0, dashPos + 1);
    ret = true;
  }
  setDefault();
  parse();
  return ret;
}

void Config::findConfig() {
  for (std::string p : defaultPaths) {
    std::string path = p + fileName;
    std::ifstream f(path.c_str());
    if (FILE *file = fopen(path.c_str(), "r")) {
      fclose(file);
      filePath = p;
      return;
    }
  }
}

std::string Config::getFilePath() {
  return filePath + fileName;
}

bool Config::validVariable(std::string var) {
  for (std::string v : variables) {
    if (v == var) return true;
  }
  return false;
}

void Config::writeToConfFile(std::string v, std::string p) {
  bool found = false;
  std::fstream* file = openFile();
  if (!file) return;

  std::vector<std::string> lines;
  std::string line;
  std::string key;
  while (getline(*file, line)) {
    int equalSignPos = line.find_first_of("=");
    if (equalSignPos == std::string::npos) {
      key = line;
    } else {
      key = line.substr(0, equalSignPos);
    }

    if (key == v) {
      found = true;
      line = v + "=" + p;
    }
    lines.push_back(line);
  }
  if (!found) {
    lines.push_back(v + "=" + p);
  }
  delete file;

  std::ofstream f(filePath + fileName, std::ofstream::trunc);
  for (std::string line : lines) {
    f << line << '\n';
  }
  f.close();
}

bool Config::set(std::string var) {
  std::fstream* file = openFile();
  if (!file) return false;

  int space = var.find_first_of(' ');
  std::string v, p;
  if (space == std::string::npos) {
    v = var;
  } else {
    v = var.substr(0, space);
    p = var.substr(space + 1);
  }
  if (validVariable(v)) {
    if (handleKeyValue(v, p)) {
      writeToConfFile(v, p);
      return true;
    }
  }
  return false;
}

