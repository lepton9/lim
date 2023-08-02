#include "../include/Clip.h"

Clip::Clip() {
  line = false;
}

Clip::Clip(bool line) : line(line) {}

void Clip::lineTrue() {
  line = true;
}

bool Clip::isLine() {
  return line;
}
