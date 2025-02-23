#include <string>

std::string strip(std::string& str);
std::string trim_beg(std::string &str);
std::string trim_end(std::string &str);
std::string trim(std::string &str);
std::string trim_c(std::string str);
void removeCharFromBeg(std::string* str, char c, int n);
bool is_integer(const std::string &s);
unsigned charToUnsigned(const char *c);
int charToInt(const char *c);
std::string alignR(std::string s, int w);
bool is_hex(std::string const &s);
unsigned concat_unsigned(unsigned x, unsigned y);
