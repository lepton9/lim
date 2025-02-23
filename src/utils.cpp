#include "../include/utils.h"
#include <regex>

std::string strip(std::string& str) {
  if (str.empty()) return str;
  for (std::string::iterator it = str.begin(); it != str.end(); ) {
    if (*it == ' ') {
      it = str.erase(it);
    } else {
      ++it;
    }
  }
  return str;
}

std::string trim_beg(std::string &str) {
  str.erase(0, str.find_first_not_of(' '));
  return str;
}

std::string trim_end(std::string &str) {
  str.erase(str.find_last_not_of(' ') + 1);
  return str;
}

std::string trim(std::string &str) {
  trim_end(str);
  trim_beg(str);
  return str;
}

std::string trim_c(std::string str) {
  str = trim_end(str);
  return trim_beg(str);
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

bool is_hex(std::string const &s) {
  return s.compare(0, 2, "0x") == 0 && s.size() > 2 &&
         s.find_first_not_of("0123456789abcdefABCDEF", 2) == std::string::npos;
}

unsigned concat_unsigned(unsigned x, unsigned y) {
  unsigned pow = 10;
  while (y >= pow) {
    pow *= 10;
  }
  return x * pow + y;
}
