#include "../include/utils.h"
#include <regex>

std::string trim(std::string &str) {
  str.erase(str.find_last_not_of(' ') + 1);
  str.erase(0, str.find_first_not_of(' '));
  return str;
}

std::string trim_c(std::string str) {
  str.erase(str.find_last_not_of(' ') + 1);
  str.erase(0, str.find_first_not_of(' '));
  return str;
}

void removeCharFromBeg(std::string* str, char c, int n) {
  for (int i = 0; i < n; i++) {
    if (!str->empty() && str->front() == c) {
      str->erase(0, 1);
    } else {
      break;
    }
  }
}

bool is_integer(const std::string &s) {
  return regex_match(s, std::regex("[+-]?[0-9]+"));
}

unsigned charToUnsigned(const char *c) {
  unsigned unsignInt = 0;
  while (*c) {
    unsignInt = unsignInt * 10 + (*c - '0');
    c++;
  }
  return unsignInt;
}

int charToInt(const char *c) {
  return (*c == '-') ? -charToUnsigned(c + 1) : charToUnsigned(c);
}

std::string alignR(std::string s, int w) {
  return std::string(w - s.length(), ' ') + s;
}

