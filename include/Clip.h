#ifndef CLIP_H
#define CLIP_H

#include <vector>
#include <string>

class Clip : public std::vector<std::string> {
  public:
    Clip();
    Clip(bool line);
    void lineTrue();
    bool isLine();
  private:
    bool line;
};

#endif
