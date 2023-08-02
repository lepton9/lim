#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include <vector>
#include "Clip.h"

class Clipboard : public std::vector<Clip> {
  public:
    void copyClip(Clip clip);

  private:
    const int maxSize = 10;
};



#endif
