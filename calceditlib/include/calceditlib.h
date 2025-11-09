#ifndef CALCEDITLIB_H
#define CALCEDITLIB_H

#include <map>
#include <string>

void calculate(
    const std::string & input,
    std::map<std::string, double> & variables,
    std::string & resultString
    );

#endif
