#include "../include/Filetree.h"
#include <algorithm>

Filetree::Filetree() {
  cX = 0;
  cY = 0;
}

Filetree::Filetree(std::string _path) {
  path = _path;
  cX = 0;
  cY = 0;
  getFilesFromPath();
}

bool Filetree::isShown() {
  return shown;
}

void Filetree::toggleShow() {
  shown ^= true;
}

void Filetree::getFilesFromPath() {
  tree.clear();

  tree.push_back({path, ".", true});
  tree.push_back({path.parent_path(), "..", true});

  for (const auto &file : std::filesystem::directory_iterator(path)) {
    std::filesystem::path fName = file.path().string().substr(file.path().string().find_last_of("/") + 1) ;
    tree.push_back({file.path(), fName, file.is_directory()});
  }
  sortTree();
}

bool compareByFileDir(const fileDir &a, const fileDir &b) {
  return a.isDir > b.isDir;
}

bool compareByName(const fileDir &a, const fileDir &b) {
  return a.name < b.name;
}

void Filetree::sortTree() {
  sort(tree.begin() + 2, tree.end(), compareByName);
  sort(tree.begin() + 2, tree.end(), compareByFileDir);
}

fileDir* Filetree::getElementOnCur() {
  if (tree.empty()) return nullptr;
  return &tree[cY];
}

void Filetree::changeDirectory(std::string _path) {
  path = _path;;
  cX = 0;
  cY = 0;
  getFilesFromPath();
}

void Filetree::refresh() {
  cX = 0;
  cY = 0;
  getFilesFromPath();
}

std::string Filetree::current_path() {
  return path;
}



