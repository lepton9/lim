#include "../include/Clipboard.h"
#include <cstddef>


void Clipboard::copyClip(Clip clip) {
  if (size() >= maxSize) {
    erase(begin());
  }
  push_back(clip);
}

