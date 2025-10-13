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

#define exprtk_disable_caseinsensitivity
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
Fl_Text_Buffer * editStyleBuffer;
TextDisplay * result;
Fl_Text_Buffer * resultBuffer;
Fl_Text_Buffer * resultStyleBuffer;
Fl_Tree * tree;

typedef exprtk::symbol_table<double> symbol_table_t;
typedef exprtk::expression<double>   expression_t;
typedef exprtk::parser<double>       parser_t;
typedef exprtk::parser<double>::settings_t settings_t;

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
  default_value = T(std::numeric_limits<double>::quiet_NaN());

  return true;
}
std::map<std::string, T> & mVariables;
};

std::string strip_comment(const std::string & line)
{
  return std::regex_replace(line, std::regex("^[ \t]*//.*"), "");
}

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
      }
      parser_input.clear();
    }
    if (i != lines.size()-1)
    {
      resultString += "\n";
    }
  }

}

void update()
{
  tree->clear();
  tree->begin();
  std::string resultString;
  std::map<std::string, double> variables;
  calculate(editBuffer->text(), variables, resultString);
  resultBuffer->text(resultString.c_str());
  for (auto &v : variables)
  {
    tree->add(("Variables/" + v.first + " = " + fmt::format("{}", v.second)).c_str()); //, std::to_string(v.second).c_str());
  }
  tree->root_label("Data");
  tree->showroot(0);
  tree->end();
  tree->damage(FL_DAMAGE_ALL);
  auto style = [](Fl_Text_Buffer * textBuffer, Fl_Text_Buffer * styleBuffer){
    std::string styleString = textBuffer->text();
    bool line = true;
    for (size_t i = 0; i < styleString.length(); i++)
    {
      if (styleString[i] != '\n')
      {
        styleString[i] = 'A' + line;
      }
      else
      {
        line = !line;
      }
    }
    styleBuffer->text(styleString.c_str());
  };
  style(editBuffer, editStyleBuffer);
  style(resultBuffer, resultStyleBuffer);
  syncVScroll(edit);
}

void editCallback(int pos, int nInserted, int nDeleted, int nRestyled, const char* deletedText, void* cbArg)
{
  update();
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

  editStyleBuffer = new Fl_Text_Buffer();
  resultStyleBuffer = new Fl_Text_Buffer();
  Fl_Text_Display::Style_Table_Entry stable[] = {
    // FONT COLOR      FONT FACE   FONT SIZE
    // --------------- ----------- --------------
    {  FL_BLACK,       FL_COURIER, 12, Fl_Text_Display::ATTR_BGCOLOR_EXT, FL_GRAY }, // A - Red
    {  FL_BLACK,       FL_COURIER, 12, Fl_Text_Display::ATTR_BGCOLOR_EXT, FL_DARK2 }, // B - Green
  };
  stable[0].bgcolor;
  edit->highlight_data(editStyleBuffer, stable, sizeof(stable)/sizeof(stable[0]), 'A', 0, 0);
  result->highlight_data(resultStyleBuffer, stable, sizeof(stable)/sizeof(stable[0]), 'A', 0, 0);

  tree = new Fl_Tree(1000, 0, 200, 800);
  tree->root_label("Variables");

  editBuffer->add_modify_callback(editCallback, 0);

  tile->end();

  win->end();

  //win->size_range(win->w(), 600);
  win->show(argc, argv);

  editBuffer->text(
    "// Comments start with double slash\n"
    "// Each non-empty line is an expression which is evaluated and the result\n"
    "// is displayed on the left pane in the same line.\n"
    "\n"
    "1 + 1\n"
    "\n"
    "// The latest result is stored in the variable \"ans\"\n"
    "\n"
    "ans + 1\n"
    "\n"
    "// Custom variables can be created simply by using them. They are initialized with \"nan\"\n"
    "// to avoid surprising results by misspelling a variable name."
    "\n"
    "1 + x\n"
    "\n"
    "// A value can be assigned explicitly by using the assignment operator \":=\"\n"
    "\n"
    "y: = 8\n"
    "\n"
    "// All variables are displayed in a tree view on the right.\n"
    "// There is one predefined constant \"pi\"\n"
    "\n"
    "cos(pi)\n"
  );

  update();

  // return Fl::run();
  int ret = Fl::run();
  delete win;
  return ret;
}
