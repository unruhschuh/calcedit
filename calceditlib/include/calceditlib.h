#ifndef CALCEDITLIB_H
#define CALCEDITLIB_H

#include <map>
#include <string>
#include <vector>

void calculate(
    const std::string & input,
    std::map<std::string, double> & variables,
    std::string & resultString
    );

std::vector<std::string> split_string_by_newline(const std::string& str);

#endif
