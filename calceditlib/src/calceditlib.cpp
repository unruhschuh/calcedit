#include "calceditlib.h"

#define exprtk_disable_caseinsensitivity
#include "exprtk_complex_adaptor.hpp"
#include "exprtk.hpp"

#include <cerrno>
#include <chrono>
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
        result = symbol_table.create_variable(unknown_symbol,T(std::numeric_limits<long double>::quiet_NaN()));

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
          mVectors[vector_name] = std::vector<T>(vector_size, T(std::numeric_limits<long double>::quiet_NaN()));
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

struct timeout_loop_rtc final : exprtk::loop_runtime_check
{
    using time_point_t =
    std::chrono::time_point<std::chrono::steady_clock>;

    std::size_t iterations_ = 0;
    time_point_t timeout_tp_;

    bool check() override
    {
      if (std::chrono::steady_clock::now() >= timeout_tp_)
      {
        // handle_runtime_violation shall be invoked
        return false;
      }

      return true;
    }

    void handle_runtime_violation
    (const exprtk::loop_runtime_check::violation_context &) override
    {
      throw std::runtime_error("Loop timed out");
    }

    void set_timeout_time(const time_point_t& timeout_tp)
    {
      timeout_tp_ = timeout_tp;
    }
};

std::string toString(cmplx::complex_t x)
{
  if (x.c_.imag() == 0.0)
  {
    return fmt::format("{}", x.c_.real());
  }
  else if (x.c_.real() == 0.0)
  {
    if (x.c_.imag() == 1.0)
      return fmt::format("i");
    else if (x.c_.imag() == -1.0)
      return fmt::format("-i");
    else
      return fmt::format("{} * i", x.c_.imag());
  }
  else
  {
    if (x.c_.imag() == 1.0)
      return fmt::format("{} + i", x.c_.real());
    else if (x.c_.imag() == -1.0)
      return fmt::format("{} - i", x.c_.real());
    else if (x.c_.imag() < 0)
      return fmt::format("{} - {} * i", x.c_.real(), abs(x.c_.imag()));
    else
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

template <typename T>
struct conj final : public exprtk::ifunction<T>
{
  conj() : exprtk::ifunction<T>(1)
  {}

  T operator()(const T& v) override
  {
    return v.conj();
  }
};

template <typename T>
struct arg final : public exprtk::ifunction<T>
{
  arg() : exprtk::ifunction<T>(1)
  {}

  T operator()(const T& v) override
  {
    return v.arg();
  }
};

template <typename T, int radix>
struct string_to_number final : public exprtk::igeneric_function<T>
{
  typedef exprtk::igeneric_function<T> igenfunct_t;
  typedef typename igenfunct_t::generic_type generic_t;
  typedef typename igenfunct_t::parameter_list_t parameter_list_t;
  typedef typename generic_t::string_view string_t;

  string_to_number()
    : exprtk::igeneric_function<T>("S")
  {}

  inline T operator()(parameter_list_t parameters) override
  {
    if (parameters.size() && parameters[0].type == exprtk::type_store<T>::e_string)
    {
      string_t tmp(parameters[0]);
      errno = 0;
      char *p_end{};
      auto result = std::strtoull(tmp.begin(), &p_end, radix);
      if (tmp.begin() == p_end || p_end != tmp.begin() + strlen(tmp.begin()) || errno == ERANGE)
      {
        return {std::numeric_limits<long double>::quiet_NaN(),0};
      }

      return {static_cast<long double>(result), 0};
    }
    return {std::numeric_limits<long double>::quiet_NaN(),0};
  }
};


template <typename T>
struct string_to_number<T, 2> final : public exprtk::igeneric_function<T>
{
  typedef exprtk::igeneric_function<T> igenfunct_t;
  typedef typename igenfunct_t::generic_type generic_t;
  typedef typename igenfunct_t::parameter_list_t parameter_list_t;
  typedef typename generic_t::string_view string_t;

  string_to_number()
    : exprtk::igeneric_function<T>("S")
  {}

  inline T operator()(parameter_list_t parameters) override
  {
    T result_error = {std::numeric_limits<long double>::quiet_NaN(),0};
    if (parameters.size() && parameters[0].type == exprtk::type_store<T>::e_string)
    {
      string_t tmp(parameters[0]);
      if (tmp.size() == 0 || tmp.size() > 64)
      {
        return result_error;
      }
      uint64_t result = 0;
      int add = 0;
      if (tmp[0] == '1')
      {
        add = 1;
      }
      for (size_t i = 0; i < tmp.size(); i++)
      {
        switch (tmp[i])
        {
          case '0':
            result = result << 1 | add;
            break;
          case '1':
            result = result << 1 | !add;
            break;
          default:
            return result_error;
        }
      }

      if (add)
      {
        result = - result - 1;
      }

      int64_t signed_result = result;

      return {static_cast<long double>(signed_result), 0};
    }
    return result_error;
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
 //                  3.1415926535897932384626433L
  variables["pi"] = {3.1415926535897932384626433L,0.0L};
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
      conj<cmplx::complex_t> conj_fun;
      arg<cmplx::complex_t> arg_fun;
      string_to_number<cmplx::complex_t, 16> hex_fun;
      string_to_number<cmplx::complex_t, 8> oct_fun;
      string_to_number<cmplx::complex_t, 2> bin_fun;
      symbol_table.add_function("real", real_fun);
      symbol_table.add_function("imag", imag_fun);
      symbol_table.add_function("conj", conj_fun);
      symbol_table.add_function("arg", arg_fun);
      symbol_table.add_function("hex", hex_fun);
      symbol_table.add_function("oct", oct_fun);
      symbol_table.add_function("bin", bin_fun);
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

      timeout_loop_rtc loop_rtc;
      loop_rtc.loop_set =  exprtk::loop_runtime_check::e_all_loops;
      loop_rtc.max_loop_iterations = 100000;

      parser_t parser;
      parser.register_loop_runtime_check(loop_rtc);
      parser.enable_unknown_symbol_resolver(&musr);
      parser.settings().disable_commutative_check();
      const auto max_duration = std::chrono::seconds(5);
      if (parser.compile(parser_input, expression))
      {
        loop_rtc.set_timeout_time(std::chrono::steady_clock::now() + max_duration);
        try {
          variables["ans"] = expression.value();
          resultString += toString(variables["ans"]);
        }
        catch(std::runtime_error& exception)
        {
          resultString += "Exception: ";
          resultString += exception.what();
        }
        //resultString += fmt::format("{} + {}", variables["ans"].c_.real(), variables["ans"].c_.imag());
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
        variables["ans"] = {std::numeric_limits<long double>::quiet_NaN(),0};
      }
      parser_input.clear();
    }
    if (i != lines.size()-1)
    {
      resultString += "\n";
    }
  }

}
