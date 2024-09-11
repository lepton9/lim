#ifndef CLIP_H
#define CLIP_H

#include <vector>
#include <string>

class LineYank {
  std::string line;
  bool fullLine;
  public:
    LineYank();
    LineYank(std::string line, bool fullLine);
    bool isFullLine();
    std::string text();
};

class Clip : public std::vector<LineYank> {
  public:
    Clip();
    Clip(LineYank ly);
};

#endif
