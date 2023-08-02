#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <fstream>

class Config {
  public:
    void parse();
    bool setFilePath(std::string path);
    bool set(std::string var);

    int indentAm = 2;
    bool curWrap = true;
    bool lineNum = true;
    bool relativenumber = false;
    bool comline_visible = false;
    int bg_color;
    int fg_color;
    int sb_bg_color;
    int sb_fg_color;

  private:
    const std::string fileName = ".lepconfig";
    std::string filePath = "./";
    std::fstream* openFile();
    void writeToConfFile(std::string v, std::string p);
    bool handleKeyValue(std::string key, std::string value);
    void setDefault();
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
