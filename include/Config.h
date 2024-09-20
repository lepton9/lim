#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>
#include <string>
#include <fstream>
#include <vector>

class Config {
  public:
    void parse();
    bool setFilePath(std::string path);
    std::string getFilePath();
    bool set(std::string var);

    int indentAm = 2;
    bool curWrap = true;
    bool lineNum = true;
    bool relativenumber = false;
    bool comline_visible = false;
    uint32_t bg_color;
    uint32_t fg_color;
    uint32_t sb_bg_color;
    uint32_t sb_fg_color;
    uint8_t r(uint32_t color);
    uint8_t g(uint32_t color);
    uint8_t b(uint32_t color);

  private:
    const std::string fileName = ".limconfig";
    std::vector<std::string> defaultPaths = {"./", "~/.config/lim/"};
    std::string filePath = "./";
    void findConfig();
    std::fstream* openFile();
    void writeToConfFile(std::string v, std::string p);
    bool handleKeyValue(std::string key, std::string value);
    void setDefault();
    bool is_hex(std::string const& s);
    bool validVariable(std::string var);
    std::string variables[9] = {
      "tab_indent", 
      "cursor_wrap",
      "number",
      "number_style",
      "comline_visible",
      "bg_color",
      "fg_color",
      "sb_bg_color",
      "sb_fg_color"
    };
};

#endif
