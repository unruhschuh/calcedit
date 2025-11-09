#include "calceditlib.h"

#define exprtk_disable_caseinsensitivity
#include "exprtk.hpp"

#include <iostream>
#include <sstream>
#include <regex>
#include <fmt/format.h>

typedef exprtk::symbol_table<double> symbol_table_t;
typedef exprtk::expression<double>   expression_t;
typedef exprtk::parser<double>       parser_t;
typedef exprtk::parser<double>::settings_t settings_t;

static bool emptyString(const std::string & s);
static std::vector<std::string> split(const std::string str, const std::string regex_str);
static std::vector<std::string> split_string_by_newline(const std::string& str);
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

static std::vector<std::string> split_string_by_newline(const std::string& str)
{
  auto result = std::vector<std::string>{};
  auto ss = std::stringstream{str};

  for (std::string line; std::getline(ss, line, '\n');)
  result.push_back(line);

  return result;
}

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

void calculate(
    const std::string & input,
    std::map<std::string, double> & variables,
    std::string & resultString
    )
{
  resultString.clear();
  variables.clear();
  variables["pi"]= 3.14159265358979323846;
  auto lines = split(input, "\n");
  std::string parser_input;
  bool block = false;
  for (size_t i = 0; i < lines.size(); i++)
  {
    const std::string &line = lines[i];
    if (line == "#block")
    {
      block = true;
    }
    else if (line == "#end")
    {
      block = false;
    }
    else
    {
      if (i + 1 == lines.size())
      {
        // close last block even if it doesn't end in "#end"
        block = false;
      }
      parser_input += strip_comment(line);
    }
    if (!emptyString(parser_input) && !block)
    {
      symbol_table_t unknown_var_symbol_table;
      symbol_table_t symbol_table;
      symbol_table.add_variable("ans", variables["ans"]);
      symbol_table.add_variable("pi", variables["pi"], true);
      for (auto & v : variables)
      {
        symbol_table.add_variable(v.first, v.second);
      }
      expression_t expression;
      expression.register_symbol_table(unknown_var_symbol_table);
      expression.register_symbol_table(symbol_table);
      my_usr<double> musr(variables);
      //parser_t parser(settings_t::compile_all_opts - settings_t::e_commutative_check);
      parser_t parser;
      parser.enable_unknown_symbol_resolver(&musr);
      if (parser.compile(parser_input, expression))
      {
        variables["ans"] = expression.value();
        resultString += fmt::format("{}", variables["ans"]);
        std::vector<std::pair<std::string,double>> variable_list;
        unknown_var_symbol_table.get_variable_list(variable_list);
        for (auto & v : variable_list)
        {
          std::cout << "unknown variable: " << v.first << std::endl;
          variables[v.first] = v.second;
        }
      }
      else
      {
        resultString += parser.error();
        variables["ans"] = std::numeric_limits<double>::quiet_NaN();
      }
      parser_input.clear();
    }
    if (i != lines.size()-1)
    {
      resultString += "\n";
    }
  }

}
