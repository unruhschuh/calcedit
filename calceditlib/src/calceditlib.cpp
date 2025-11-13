#include "calceditlib.h"

#define exprtk_disable_caseinsensitivity
#include "exprtk_complex_adaptor.hpp"
#include "exprtk.hpp"

#include <iostream>
#include <sstream>
#include <regex>
#include <fmt/format.h>

#if 0
typedef exprtk::symbol_table<double> symbol_table_t;
typedef exprtk::expression<double>   expression_t;
typedef exprtk::parser<double>       parser_t;
typedef exprtk::parser<double>::settings_t settings_t;
#else
typedef exprtk::symbol_table<cmplx::complex_t> symbol_table_t;
typedef exprtk::expression<cmplx::complex_t>   expression_t;
typedef exprtk::parser<cmplx::complex_t>       parser_t;
typedef exprtk::parser<cmplx::complex_t>::settings_t settings_t;
#endif

static bool emptyString(const std::string & s);
static std::vector<std::string> split(const std::string str, const std::string regex_str);
static std::string strip_comment(const std::string & line);

static bool emptyString(const std::string & s)
{
  for (auto c : s)
  {
    if (!(c == ' ' || c == '\t')) return false;
  }
  return true;
}

static std::string strip_comment(const std::string & line)
{
  return std::regex_replace(line, std::regex("^[ \t]*//.*"), "");
}

static std::vector<std::string> split(const std::string str, const std::string regex_str)
{
    std::regex regexz(regex_str);
    std::vector<std::string> list(std::sregex_token_iterator(str.begin(), str.end(), regexz, -1),
                                  std::sregex_token_iterator());


    if (str.size() && str.back() == '\n')
      list.push_back("");
    return list;
}

std::vector<std::string> split_string_by_newline(const std::string& str)
{
  auto result = std::vector<std::string>{};
  auto ss = std::stringstream{str};

  for (std::string line; std::getline(ss, line, '\n');)
  result.push_back(line);

  return result;
}

#if 0
template <typename T>
struct my_usr final : public parser_t::unknown_symbol_resolver
{
typedef typename parser_t::unknown_symbol_resolver usr_t;

    my_usr(std::map<std::string, T> & variables) : mVariables{variables}
    {}

bool process(const std::string& unknown_symbol,
             typename usr_t::usr_symbol_type& st,
             T& default_value,
             std::string& error_message) override
{
  //if (0 != unknown_symbol.find("var_"))
  //{
  //  error_message = "Invalid symbol: " + unknown_symbol;
  //  return false;
  //}

  st = usr_t::e_usr_variable_type;
  default_value = T(std::numeric_limits<double>::quiet_NaN());

  return true;
}
std::map<std::string, T> & mVariables;
};
#else
template <typename T>
struct my_usr final : public parser_t::unknown_symbol_resolver
{
    typedef typename parser_t::unknown_symbol_resolver usr_t;

    my_usr(std::map<std::string, T> & variables, std::map<std::string, std::vector<T>> & vectors)
      : mVariables{variables}, mVectors{vectors}, usr_t(usr_t::e_usrmode_extended)
    {}

    bool process(const std::string& unknown_symbol,
                 symbol_table_t&    symbol_table,
                 std::string&       error_message) override
    {
      bool result = false;

      if (1) // (std::islower(unknown_symbol[0]))
      {
        // Default value of zero
        result = symbol_table.create_variable(unknown_symbol,T(std::numeric_limits<double>::quiet_NaN()));

        if (!result)
        {
          error_message = "Failed to create variable...";
        }
      }
      else if (std::isupper(unknown_symbol[0]))
      {
        auto pos = unknown_symbol.find_last_of('_');
        if (pos == std::string::npos || pos+1 == unknown_symbol.length() || !std::isdigit(unknown_symbol[pos+1]))
        {
          result = false;
        }
        else
        {
          size_t vector_size = std::stoul(unknown_symbol.substr(pos+1));
          auto vector_name = unknown_symbol.substr(0, pos);
          mVectors[vector_name] = std::vector<T>(vector_size, T(std::numeric_limits<double>::quiet_NaN()));
          result = symbol_table.add_vector(unknown_symbol, mVectors[vector_name]);
          // Default value of empty string
          //result = symbol_table.create_stringvar(unknown_symbol,"");
        }

        if (!result)
        {
          error_message = "Failed to create vector variable...";
        }
      }
      else
        error_message = "Indeterminable symbol type.";

      return result;
    }
  std::map<std::string, T> & mVariables;
  std::map<std::string, std::vector<T>> & mVectors;
};
#endif

std::string toString(cmplx::complex_t x)
{
  if (x.c_.imag() == 0.0)
  {
    return fmt::format("{}", x.c_.real());
  }
  else
  {
    return fmt::format("{} + {} * i", x.c_.real(), x.c_.imag());
  }
}

template <typename T>
struct real final : public exprtk::ifunction<T>
{
  real() : exprtk::ifunction<T>(1)
  {}

  T operator()(const T& v) override
  {
    return T(v.real());
  }
};

template <typename T>
struct imag final : public exprtk::ifunction<T>
{
  imag() : exprtk::ifunction<T>(1)
  {}

  T operator()(const T& v) override
  {
    return T(v.imag());
  }
};

void calculate(
    const std::string & input,
    std::map<std::string, cmplx::complex_t> & variables,
    std::map<std::string, std::vector<cmplx::complex_t>> & vectors,
    std::string & resultString
    )
{
  resultString.clear();
  vectors.clear();
  variables.clear();
  variables["pi"] = {3.14159265358979323846,0};
  auto lines = split(input, "\n");
  std::string parser_input;
  bool block = false;
  for (size_t i = 0; i < lines.size(); i++)
  {
    const std::string &line = lines[i];
    parser_input += line + "\n"; // strip_comment(line) + " ";
    if ( (line.length() >= 2 && line.substr(0,2) == "##" ) ||
         (line.length() >= 3 && line.substr(0,3) == "///")    )
    {
      block = true;
    }
    if (i + 1 >= lines.size() || emptyString(lines[i+1]))
    {
      block = false;
    }
    if (!emptyString(parser_input) && !block)
    {
      symbol_table_t unknown_var_symbol_table;
      symbol_table_t symbol_table;
      cmplx::complex_t const_i = cmplx::complex_t(0.0,1.0);
      symbol_table.add_constant("i", const_i);
      symbol_table.add_variable("ans", variables["ans"]);
      symbol_table.add_variable("pi", variables["pi"], true);
      real<cmplx::complex_t> real_fun;
      imag<cmplx::complex_t> imag_fun;
      symbol_table.add_function("real", real_fun);
      symbol_table.add_function("imag", imag_fun);
      //symbol_table.add_constants();
      for (auto & v : variables)
      {
        symbol_table.add_variable(v.first, v.second);
      }
      for (auto & v : vectors)
      {
        symbol_table.add_vector(v.first, v.second);
      }
      expression_t expression;
      expression.register_symbol_table(unknown_var_symbol_table);
      expression.register_symbol_table(symbol_table);
      my_usr<cmplx::complex_t> musr(variables, vectors);
      //parser_t parser(settings_t::compile_all_opts - settings_t::e_commutative_check);
      parser_t parser;
      parser.enable_unknown_symbol_resolver(&musr);
      parser.settings().disable_commutative_check();
      if (parser.compile(parser_input, expression))
      {
        variables["ans"] = expression.value();
        //resultString += fmt::format("{} + {}", variables["ans"].c_.real(), variables["ans"].c_.imag());
        resultString += toString(variables["ans"]);
        std::vector<std::pair<std::string,cmplx::complex_t>> variable_list;
        unknown_var_symbol_table.get_variable_list(variable_list);
        for (auto & v : variable_list)
        {
          variables[v.first] = v.second;
        }
        // TODO: exprtk::details::base_function_list
        std::vector<std::string> vector_list;
        unknown_var_symbol_table.get_vector_list(vector_list);
        //for (auto & vector_name : vector_list)
        //{
        //  auto vector = unknown_var_symbol_table.get_vector(vector_name);
        //  auto & v = vectors[vector_name];
        //  for (size_t i = 0; i < vector->size(); i++)
        //  {
        //    v.push_back(vector->data()[i]);
        //  }
        //}
      }
      else if ( ! parser.lexer().empty() )
      {
        resultString += parser.error();
        variables["ans"] = {std::numeric_limits<double>::quiet_NaN(),0};
      }
      parser_input.clear();
    }
    if (i != lines.size()-1)
    {
      resultString += "\n";
    }
  }

}
