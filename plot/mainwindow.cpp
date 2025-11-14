#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "savewavdialog.h"

#include <QScrollBar>
#include <QMessageBox>

#define exprtk_disable_caseinsensitivity
#include "exprtk.hpp"

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
  , ui(new Ui::MainWindow)
{
  ui->setupUi(this);

  connect(ui->plainTextEdit, &QPlainTextEdit::textChanged, this, &MainWindow::updateCalculation);
  //connect(ui->widget->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(updateCalculation()));
  //connect(ui->widget->yAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(updateCalculation()));
  connect(ui->widget->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(startCalculationTimer()));
  connect(ui->widget->yAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(startCalculationTimer()));

  connect(ui->actionSave_WAV, &QAction::triggered, this, &MainWindow::saveWav);
  connect(ui->actionSave_Pdf, &QAction::triggered, [this](){
    auto fileName = QFileDialog::getSaveFileName(this);
    if (!fileName.isEmpty())
    {
      if (!ui->widget->savePdf(fileName))
      {
        QMessageBox::critical(this, "Error", "Failed to save PDF");
      }
    }
  });
  connect(ui->actionSave_Png, &QAction::triggered, [this](){
    auto fileName = QFileDialog::getSaveFileName(this);
    if (!fileName.isEmpty())
    {
      if (!ui->widget->savePng(fileName))
      {
        QMessageBox::critical(this, "Error", "Failed to save PNG");
      }
    }
  });

  connect(&m_recalcTimer, &QTimer::timeout, this, &MainWindow::updateCalculation);

  ui->widget->setInteraction(QCP::iRangeDrag, true);
  ui->widget->setInteraction(QCP::iRangeZoom, true);
  ui->widget->xAxis->setRange(-0.5, 3.5);
  ui->widget->yAxis->setRange(-3.5, 3.5);

  ui->widget->installEventFilter(this);
  ui->widget->setNoAntialiasingOnDrag(true);

  m_recalcTimer.setSingleShot(true);
  m_title = new QCPTextElement(ui->widget, "", QFont("sans", 12, QFont::Bold));
  ui->widget->plotLayout()->insertRow(0);
  ui->widget->plotLayout()->addElement(0, 0, m_title);

  ui->plainTextEdit->setPlainText(
    "// Comments start with // or #\n"
    "// Set title and labels\n"
    "\n"
    "title := 'Example';\n"
    "label_x := 'time';\n"
    "label_y := 'amplitude';\n"
    "\n"
    "// Use built in functions like sin(), cos(), etc.\n"
    "// Any variable a value is asigned to via := ends up in the graph. Variable names are case-sensitive.\n"
    "\n"
    "Sin := 2+sin(x*pi);\n"
    "\n"
    "// Local variables can be created with \"var\".\n"
    "\n"
    "var rel_x := x-floor(x);\n"
    "\n"
    "// If statements can be used to define piece-wise functions.\n"
    "if (floor(x)%2)\n"
    "{\n"
    "  Rect:= -3;\n"
    "  Par:= (2*rel_x-1)^2-1;\n"
    "}\n"
    "else\n"
    "{\n"
    "  Rect:=-1;\n"
    "  Par:=1-(2*rel_x-1)^2;\n"
    "};\n"
    "\n"
    "// if you want a function to have a gap, simply assign \"inf\".\n"
    "if (x > 2.25 and x < 2.75)\n"
    "{\n"
    "  Par := inf;\n"
    "  Sin := inf;\n"
    "  Rect := inf;\n"
    "};\n"

  );
}

MainWindow::~MainWindow()
{
  delete ui;
}

///////////////////////////////////

typedef exprtk::symbol_table<double> symbol_table_t;
typedef exprtk::expression<double>   expression_t;
typedef exprtk::parser<double>       parser_t;
typedef exprtk::parser<double>::settings_t settings_t;

void MainWindow::startCalculationTimer()
{
  m_recalcTimer.start(200);
}

#if 0
static const std::vector<QColor> graphColors = {
  QColor(Qt::red),
  QColor(Qt::green),
  QColor(Qt::blue),
  QColor(Qt::cyan),
  QColor(Qt::magenta),
  QColor(Qt::yellow),
  QColor(Qt::darkRed),
  QColor(Qt::darkGreen),
  QColor(Qt::darkBlue),
  QColor(Qt::darkCyan),
  QColor(Qt::darkMagenta),
  QColor(Qt::darkYellow)
};
#else
static const std::vector<QColor> graphColors = {
  QColor("#cc241d"),
  QColor("#98971a"),
  QColor("#d79921"),
  QColor("#458588"),
  QColor("#b16286"),
  QColor("#689d6a"),
  QColor("#7c6f64"),
  QColor("#d65d0e")
};
#endif

template <typename T>
struct my_usr final : public parser_t::unknown_symbol_resolver
{
typedef typename parser_t::unknown_symbol_resolver usr_t;

    my_usr(std::vector<std::string> & variable_list) : m_variable_list{variable_list}
    {}

bool process(const std::string& unknown_symbol,
             typename usr_t::usr_symbol_type& st,
             T& default_value,
             std::string& error_message) override
{
  st = usr_t::e_usr_variable_type;
  default_value = T(std::numeric_limits<double>::quiet_NaN());

  m_variable_list.push_back(unknown_symbol);

  return true;
}
std::vector<std::string> & m_variable_list;
};

struct Parser
{
  Parser()
  {
    symbol_table.add_variable("x", x);
    symbol_table.add_constants();
    symbol_table.add_stringvar("title", title);
    symbol_table.add_stringvar("label_x", label_x);
    symbol_table.add_stringvar("label_y", label_y);

    expression.register_symbol_table(unknown_var_symbol_table);
    expression.register_symbol_table(symbol_table);

    parser.enable_unknown_symbol_resolver(usr);
    parser.settings().disable_commutative_check();
  };
    double x;
    symbol_table_t unknown_var_symbol_table;
    symbol_table_t symbol_table;
    expression_t expression;
    parser_t parser;
    std::vector<std::string> variable_list;
    my_usr<double> usr{variable_list};
    std::string title;
    std::string label_x;
    std::string label_y;
};

void MainWindow::updateCalculation()
{
  Parser parser;

  auto parser_input = ui->plainTextEdit->toPlainText().toStdString();

  //bool found = false;
  //auto range = ui->widget->graph()->getValueRange(found);
  auto range = ui->widget->xAxis->range();
  {
    auto rangeWidth = range.upper - range.lower;
    range.lower -= rangeWidth;
    range.upper += rangeWidth;
  }

  if (parser.parser.compile(parser_input, parser.expression))
  {
    auto customPlot = ui->widget;
    customPlot->clearGraphs();

    int N = std::max(ui->widget->width()*2, 100);

    struct Variable
    {
      std::string name;
      double *value;
      QVector<double> Y;
    };

    std::vector<Variable> variables;

    for (const auto & v : parser.variable_list)
    {
      //variables[v] = &unknown_var_symbol_table.variable_ref(v);
      variables.push_back({v, &parser.unknown_var_symbol_table.variable_ref(v), QVector<double>(N)});
    }
    QVector<double> X(N);

    double expr_value = 0;

    if (variables.empty())
    {
      variables.push_back({"y", &expr_value, QVector<double>(N)});
    }

    for (int i=0; i<N; ++i)
    {
      X[i] = i * (range.upper - range.lower) / (double)(N-1) + range.lower;
      parser.x = X[i];
      expr_value = parser.expression.value();
      for (auto &v : variables)
      {
        v.Y[i] = *(v.value);
      }
    }
    for (auto &v : variables)
    {
      // create graph and assign data to it:
      customPlot->addGraph();
      auto graphIndex = customPlot->graphCount()-1;
      auto graph = customPlot->graph(graphIndex);
      graph->setAntialiased(true);
      //graph->setData(X, Y[v.first]);
      graph->setData(X, v.Y);
      QString name = v.name.c_str();
      name.replace('_', ' ');
      graph->setName(name);
      graph->setPen(QPen(graphColors[graphIndex % graphColors.size()]));
    }

    customPlot->xAxis->setLabel("x");
    customPlot->yAxis->setLabel("y");
    if (!parser.label_x.empty()) {
      customPlot->xAxis->setLabel(parser.label_x.c_str());
    }
    if (!parser.label_y.empty()) {
      customPlot->yAxis->setLabel(parser.label_y.c_str());
    }
    m_title->setText(parser.title.c_str());
    customPlot->legend->setVisible(true);
    customPlot->replot();
    statusBar()->clearMessage();
  }
  else if ( ! parser.parser.lexer().empty() )
  {
    //statusBar()->showMessage(parser.error().c_str());
    typedef exprtk::parser_error::type error_t;

    error_t error = parser.parser.get_error(0);

    exprtk::parser_error::update_error(error,parser_input);

    int i = 0;
    QString error_message = QString("Error[%1] at line: %2 column: %3").arg(i).arg(error.line_no).arg(error.column_no);
    statusBar()->showMessage(error_message);

  }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::Wheel)
  {
    if (QGuiApplication::keyboardModifiers() & Qt::ShiftModifier)
    {
      ui->widget->axisRect()->setRangeZoom(Qt::Horizontal);
    }
    else if (QGuiApplication::keyboardModifiers() & Qt::ControlModifier)
    {
      ui->widget->axisRect()->setRangeZoom(Qt::Vertical);
    }
    else
    {
      ui->widget->axisRect()->setRangeZoom(Qt::Vertical | Qt::Horizontal);
    }
  }
  return QObject::eventFilter(obj, event);
}

static void writeMonoWav(FILE * fp, float * data, size_t N, int channels, int loop)
{
  uint32_t rate = 48000;  // Sample rate
  uint32_t frame_count = N * loop;
  uint16_t chan_num = channels;  // Number of channels
  uint16_t bits = 16;  // Bit depth
  uint32_t length = frame_count*chan_num*bits / 8;
  int16_t byte;
  // scales signal to use full 16bit resolution
  float multiplier = pow(2, bits-1)-1; // /* 32767

  //// WAVE Header Data
  fwrite("RIFF", 1, 4, fp);
  uint32_t chunk_size = length + 44 - 8;
  fwrite(&chunk_size, 4, 1, fp);
  fwrite("WAVE", 1, 4, fp);
  fwrite("fmt ", 1, 4, fp);
  uint32_t subchunk1_size = 16;
  fwrite(&subchunk1_size, 4, 1, fp);
  uint16_t fmt_type = 1;  // 1 = PCM
  fwrite(&fmt_type, 2, 1, fp);
  fwrite(&chan_num, 2, 1, fp);
  fwrite(&rate, 4, 1, fp);
  // (Sample Rate * BitsPerSample * Channels) / 8
  uint32_t byte_rate = rate * bits * chan_num / 8;
  fwrite(&byte_rate, 4, 1, fp);
  uint16_t block_align = chan_num * bits / 8;
  fwrite(&block_align, 2, 1, fp);
  fwrite(&bits, 2, 1, fp);

  // Marks the start of the data
  fwrite("data", 1, 4, fp);
  fwrite(&length, 4, 1, fp);  // Data size
  for (int l = 0; l < loop; l++)
  {
    for (uint32_t i = 0; i < N; i++)
    {
      for (int c = 0; c < chan_num; c++)
      {
        byte = (data[i * channels + c] * multiplier);
        fwrite(&byte, 2, 1, fp);
      }
    }
  }
}

void MainWindow::saveWav()
{
  auto d = new SaveWavDialog(this);
  if (QDialog::Accepted == d->exec())
  {
    auto fileName = QFileDialog::getSaveFileName(this);
    if (!fileName.isEmpty())
    {
      FILE * f = fopen(fileName.toUtf8().constData(), "wb");
      if (f)
      {
        Parser parser;
        auto parser_input = ui->plainTextEdit->toPlainText().toStdString();
        if (parser.parser.compile(parser_input, parser.expression))
        {
          double from = d->from();
          double to = d->to();
          size_t N = d->N();

          auto channels = parser.variable_list.size();
          std::vector<float> data(N * channels);

          for (size_t i = 0; i < N; i++)
          {
            parser.x = i * (to-from) / (double)(N-1) + from;
            parser.expression.value();
            for (size_t n = 0; n < channels; n++)
            {
              double & y = parser.unknown_var_symbol_table.variable_ref(parser.variable_list[n]);
              data[i * channels + n] = y;
            }
          }
          writeMonoWav(f, data.data(), N, channels, d->loop());
          fclose(f);
        }
      }
    }
  }
}
