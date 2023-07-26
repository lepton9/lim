#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include <vector>
#include "Clip.h"

class Clipboard : public std::vector<Clip> {
  private:
    const int maxSize = 10;
    void copyClip(Clip clip);
};



#endif
