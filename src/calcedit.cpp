#include <iostream>
#include <sstream>

//#include <fmt/chrono.h>
//#include <fmt/format.h>

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Input.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Text_Editor.H>

#include "exprtk.hpp"

class TextEdit : public Fl_Text_Editor
{
  public:
    TextEdit(int X, int Y, int W, int H, const char* l = 0) : Fl_Text_Editor(X, Y, W, H, l)
    {}

    ~TextEdit()
    {
    }

    int topLineNum()
    {
      return mTopLineNum;
    }
};

class TextDisplay : public Fl_Text_Display
{
  public:
    TextDisplay(int X, int Y, int W, int H, const char* l = 0) : Fl_Text_Display(X, Y, W, H, l)
    {}

};

TextEdit * edit;
Fl_Text_Buffer * editBuffer;
TextDisplay * result;
Fl_Text_Buffer * resultBuffer;

typedef exprtk::symbol_table<double> symbol_table_t;
typedef exprtk::expression<double>   expression_t;
typedef exprtk::parser<double>       parser_t;

std::vector<std::string> split_string_by_newline(const std::string& str)
{
  auto result = std::vector<std::string>{};
  auto ss = std::stringstream{str};

  for (std::string line; std::getline(ss, line, '\n');)
  result.push_back(line);

  return result;
}

bool emptyString(const std::string & s)
{
  if (s.size() && s[0] == '#') return true;
  for (auto c : s)
  {
    if (!(c == ' ' || c == '\t')) return false;
  }
  return true;
}

void editCallback(int pos, int nInserted, int nDeleted, int nRestyled, const char* deletedText, void* cbArg)
{
  std::cout << "Callback\n";
  auto lines = split_string_by_newline(editBuffer->text());
  std::string resultString;
  double ans = 0;
  std::map<std::string, double> variables;
  double pi = 3.14159265358979323846;
  for (auto & line : lines)
  {
    if (emptyString(line))
    {
      resultString += "\n";
      continue;
    }
    symbol_table_t symbol_table;
    symbol_table.add_variable("ans",ans);
    symbol_table.add_variable("pi",pi);
    for (char c = 'a' ; c <= 'z'; c++)
    {
      symbol_table.add_variable(std::string()+c,variables[std::string()+c]);
    }
    for (char c = 'A' ; c <= 'Z'; c++)
    {
      symbol_table.add_variable(std::string()+c,variables[std::string()+c]);
    }
    expression_t expression;
    expression.register_symbol_table(symbol_table);
    parser_t parser;
    if (parser.compile(line, expression))
    {
      ans = expression.value();
      resultString += std::to_string(ans) + "\n";
    }
    else
    {
      resultString += "!\n";
    }
  }
  resultBuffer->text(resultString.c_str());
  //resultBuffer->text(editBuffer->text());
}

void scrollCallback(Fl_Widget * w)
{
  std::cout << __FUNCTION__ << "\n";
}

void watcher(void*)
{
  result->scroll(edit->topLineNum(), 0);
  Fl::repeat_timeout(0.01, watcher, nullptr);
}

int main(int argc, char **argv)
{
  Fl::repeat_timeout(0.01, watcher, nullptr);
  Fl::scheme("gtk+");
  int pad = 10;
  Fl_Window *win = new Fl_Window(1200, 800, "Calcedit");

  win->begin();

  // create the buttons

  resultBuffer = new Fl_Text_Buffer();
  result = new TextDisplay(10, 10, 200, 780);
  result->buffer(resultBuffer);
  //result->linenumber_width(50);
  result->linenumber_format("%d");
  result->align(FL_ALIGN_RIGHT);
  result->callback(scrollCallback);

  editBuffer = new Fl_Text_Buffer();
  edit = new TextEdit(220, 10, 1200-230, 780);
  edit->buffer(editBuffer);
  //edit->linenumber_width(50);
  edit->linenumber_format("%d");
  edit->callback(scrollCallback);

  editBuffer->add_modify_callback(editCallback, 0);

  win->end();

  //win->size_range(win->w(), 600);
  win->show(argc, argv);

  // return Fl::run();
  int ret = Fl::run();
  delete win;
  return ret;
}
