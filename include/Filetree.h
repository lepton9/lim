#ifndef FILETREE_H
#define FILETREE_H

#include <filesystem>
#include <vector>
#include <string>

typedef struct fileDir {
  std::filesystem::path path;
  std::string name;
  bool isDir;
} fileDir;

class Filetree {
  public:
    Filetree();
    Filetree(std::string _path);
    std::vector<fileDir> tree; 
    bool isShown();
    void toggleShow();
    fileDir* getElementOnCur();
    void changeDirectory(std::string path);
    int cX, cY;
    int treeWidth = 30;
  private:
    std::filesystem::path path;
    void getFilesFromPath();
    void sortTree();
    bool shown = false;
};

#endif
