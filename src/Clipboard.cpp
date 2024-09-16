#include "../include/Clipboard.h"

void Clipboard::copyClip(Clip clip) {
  if (size() >= maxSize) {
    pop_back();
  }
  insert(this->begin(), clip);
}

