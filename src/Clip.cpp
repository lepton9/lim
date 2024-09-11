#include "../include/Clip.h"

LineYank::LineYank() {
  fullLine = false;
  line = "";
}

LineYank::LineYank(std::string line, bool fullLine) {
  this->fullLine = fullLine;
  this->line = line;
}

bool LineYank::isFullLine() {
  return fullLine;
}

std::string LineYank::text() {
  return line;
}

Clip::Clip() {}

Clip::Clip(LineYank ly) {
  this->push_back(ly);
}

