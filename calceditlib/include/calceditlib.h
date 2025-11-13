#ifndef CALCEDITLIB_H
#define CALCEDITLIB_H

#include <map>
#include <string>
#include <vector>

#include <complex_type.hpp>

void calculate(
    const std::string & input,
    std::map<std::string, cmplx::complex_t> & variables,
    std::map<std::string, std::vector<cmplx::complex_t>> & vectors,
    std::string & resultString
    );

std::string toString(cmplx::complex_t);

std::vector<std::string> split_string_by_newline(const std::string& str);

#endif
