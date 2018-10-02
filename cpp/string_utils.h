#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>

using namespace std;

string join(vector<string> strings, string separator = ",");
string format(double d);
string format(double d, int width, int precision);
vector<string> split(const string& s, char delim_char=',');


// trim from start (in place)
void ltrim(std::string &s);

// trim from end (in place)
void rtrim(std::string &s);

// trim from both ends (in place)
void trim(std::string &s);

// trim from start (copying)
std::string ltrimmed(std::string s);

// trim from end (copying)
std::string rtrimmed(std::string s);

// trim from both ends (copying)
std::string trimmed(std::string s);


#endif // STRING_UTILS_H
