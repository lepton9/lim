#ifndef CLIP_H
#define CLIP_H

#include <vector>
#include <string>

class Clip : public std::vector<std::string> {
  public:
    Clip(bool line);
  private:
    bool isLine;
};

#endif
