#include <iostream>
#include <sstream>
#include <regex>

//#include <fmt/chrono.h>
#include <fmt/format.h>

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Input.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Tree.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Tile.H>

#include "exprtk.hpp"

void syncVScroll(void * me);

class TextEdit : public Fl_Text_Editor
{
  public:
    TextEdit(int X, int Y, int W, int H, const char* l = 0) : Fl_Text_Editor(X, Y, W, H, l)
    {
      mVScrollBar->callback((Fl_Callback*)v_scrollbar_cb_custom, this);
    }

    ~TextEdit()
    {
    }

    int topLineNum()
    {
      return mTopLineNum;
    }

    int horizOffset()
    {
      return  mHorizOffset;
    }

    static void 	v_scrollbar_cb_custom (Fl_Scrollbar *w, Fl_Text_Display *d)
    {
      v_scrollbar_cb(w, d);
      syncVScroll(d);
    }
};

class TextDisplay : public Fl_Text_Display
{
  public:
    TextDisplay(int X, int Y, int W, int H, const char* l = 0) : Fl_Text_Display(X, Y, W, H, l)
    {
      mVScrollBar->callback((Fl_Callback*)v_scrollbar_cb_custom, this);
    }

    int topLineNum()
    {
      return mTopLineNum;
    }

    int horizOffset()
    {
      return  mHorizOffset;
    }

    static void 	v_scrollbar_cb_custom (Fl_Scrollbar *w, Fl_Text_Display *d)
    {
      v_scrollbar_cb(w, d);
      syncVScroll(d);
    }
};

TextEdit * edit;
Fl_Text_Buffer * editBuffer;
TextDisplay * result;
Fl_Text_Buffer * resultBuffer;
Fl_Tree * tree;

typedef exprtk::symbol_table<double> symbol_table_t;
typedef exprtk::expression<double>   expression_t;
typedef exprtk::parser<double>       parser_t;

void syncVScroll(void * me)
{
  if (me == edit)
  {
    result->scroll(edit->topLineNum(), result->horizOffset());
  }
  else
  {
    edit->scroll(result->topLineNum(), edit->horizOffset());
  }
}

std::vector<std::string> split(const std::string str, const std::string regex_str)
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

bool emptyString(const std::string & s)
{
  if (s.size() && s[0] == '#') return true;
  for (auto c : s)
  {
    if (!(c == ' ' || c == '\t')) return false;
  }
  return true;
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
  default_value = T(0);

  return true;
}
std::map<std::string, T> & mVariables;
};

void editCallback(int pos, int nInserted, int nDeleted, int nRestyled, const char* deletedText, void* cbArg)
{
  tree->clear();
  tree->begin();
  auto lines = split(editBuffer->text(), "\n");
  std::string resultString;
  std::map<std::string, double> variables;
  variables["pi"]= 3.14159265358979323846;
  for (size_t i = 0; i < lines.size(); i++)
  {
    const std::string &line = lines[i];
    if (!emptyString(line))
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
      parser_t parser;
      parser.enable_unknown_symbol_resolver(&musr);
      if (parser.compile(line, expression))
      {
        variables["ans"] = expression.value();
        resultString += fmt::format("{}", variables["ans"]);
        std::vector<std::pair<std::string,double>> variable_list;
        unknown_var_symbol_table.get_variable_list(variable_list);
        for (auto & v : variable_list)
        {
          variables[v.first] = v.second;
        }
      }
      else
      {
        resultString += parser.error();
      }
    }
    if (i != lines.size()-1)
      resultString += "\n";
  }
  for (auto &v : variables)
  {
  //tree->add((v.first + "/" + std::to_string(line_number) + " : " + std::to_string(v.second)).c_str()); //, std::to_string(v.second).c_str());
  tree->add((v.first + " = " + fmt::format("{}", v.second)).c_str()); //, std::to_string(v.second).c_str());
  }
  resultBuffer->text(resultString.c_str());
  tree->root_label("Variables");
  tree->end();
  tree->damage(FL_DAMAGE_ALL);
  //resultBuffer->text(editBuffer->text());
  syncVScroll(edit);
}

void window_callback(Fl_Widget*, void*) {
  if (Fl::event()==FL_SHORTCUT && Fl::event_key()==FL_Escape)
    return; // ignore Escape
  exit(0);
}

int main(int argc, char **argv)
{
  Fl::scheme("gtk+");
  int pad = 10;
  Fl_Window *win = new Fl_Window(1200, 800, "Calcedit");
  win->resizable(win);
  win->callback(window_callback);

  auto tile = new Fl_Tile(0, 0, 1200, 800);
  tile->init_size_range(100, 50);

  auto box = new Fl_Box(0, 0, 1200, 800);
  tile->resizable(box);

  resultBuffer = new Fl_Text_Buffer();
  result = new TextDisplay(0, 0, 200, 800);
  result->buffer(resultBuffer);
  result->linenumber_width(40);
  result->linenumber_format("%d");
  result->align(FL_ALIGN_RIGHT);

  editBuffer = new Fl_Text_Buffer();
  edit = new TextEdit(200, 0, 800, 800);
  edit->buffer(editBuffer);
  edit->linenumber_width(40);
  edit->linenumber_format("%d");

  tree = new Fl_Tree(1000, 0, 200, 800);
  tree->root_label("Variables");

  editBuffer->add_modify_callback(editCallback, 0);

  tile->end();

  win->end();

  //win->size_range(win->w(), 600);
  win->show(argc, argv);

  // return Fl::run();
  int ret = Fl::run();
  delete win;
  return ret;
}
