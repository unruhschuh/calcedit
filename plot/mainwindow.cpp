#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "savewavdialog.h"

#include <QScrollBar>
#include <QMessageBox>

#define exprtk_disable_caseinsensitivity
#include "exprtk.hpp"

#include <regex>

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

  connect(ui->actionApply_limits_and_ratios, &QAction::triggered, this, &MainWindow::updateCalculation);
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

  ui->widget->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom); // | QCP::iSelectAxes | QCP::iSelectLegend | QCP::iSelectPlottables);
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
    "title('Example');\n"
    "labelX('time');\n"
    "labelY('amplitude');\n"
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
    "xLim(0,3*pi);\n"
    "yLim(-3.5,3.5);\n"
    "xAxisPi(1);\n"
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
    template <typename T>
    struct String final : public exprtk::igeneric_function<T>
    {
        typedef exprtk::igeneric_function<T> igenfunct_t;
        typedef typename igenfunct_t::generic_type generic_t;
        typedef typename igenfunct_t::parameter_list_t parameter_list_t;
        typedef typename generic_t::string_view string_t;

        String(std::string & title)
                : m_title(title), exprtk::igeneric_function<T>("S")
        {}

        inline T operator()(parameter_list_t parameters) override
        {
          if (parameters.size())
          {
            exprtk::type_store<T> & arg = parameters[0];
            string_t tmp(parameters[0]);
            m_title.clear();
            for (int i = 0; i < tmp.size(); i++)
            {
              m_title += tmp[i];
            }
          }
          return 0;
        }

        std::string & m_title;
    };
    template <typename T>
    struct Lim final : public exprtk::ifunction<T>
    {
        Lim(std::optional<std::pair<T,T>> & limits) : m_limits{limits}, exprtk::ifunction<T>(2)
        {}

        T operator()(const T& v1, const T& v2) override
        {
          m_limits = {v1, v2};
          return v2 - v1;
        }
        std::optional<std::pair<T,T>> &m_limits;
    };

    template <typename T>
    struct Bool final : public exprtk::ifunction<T>
    {
        Bool(std::optional<bool> & value) : m_value{value}, exprtk::ifunction<T>(1)
        {}

        T operator()(const T& v1) override
        {
          m_value = v1;
          return 0;
        }
        std::optional<bool> & m_value;
    };

    template <typename T>
    struct Double final : public exprtk::ifunction<T>
    {
        Double(std::optional<double> & value) : m_value{value}, exprtk::ifunction<T>(1)
        {}

        T operator()(const T& v1) override
        {
          m_value = v1;
          return 0;
        }
        std::optional<double> & m_value;
    };

#if 0
    template <typename T>
    struct Ratio final : public exprtk::ifunction<T>
    {
        Ratio(std::vector<std::tuple<double,double,double>> & ratios) : m_ratios{ratios}, exprtk::ifunction<T>(3)
        {}

        T operator()(const T& v1, const T& v2, const T& v3) override
        {
          m_ratios.push_back({v1,v2,v3});
          return 0;
        }
        std::vector<std::tuple<double,double,double>> & m_ratios;
    };
#endif

    Parser()
    {
      symbol_table.add_variable("x", x);
      symbol_table.add_constants();
      symbol_table.add_function("title", title_fun);
      symbol_table.add_function("xLabel", labelX_fun);
      symbol_table.add_function("yLabel", labelY_fun);
      symbol_table.add_function("xLabel2", labelX2_fun);
      symbol_table.add_function("yLabel2", labelY2_fun);
      symbol_table.add_function("xLim", xlim_fun);
      symbol_table.add_function("yLim", ylim_fun);
      symbol_table.add_function("xLim2", xlim2_fun);
      symbol_table.add_function("yLim2", ylim2_fun);
      symbol_table.add_function("axisRatio", axisRatio_fun);
      symbol_table.add_function("axisRatio2", axisRatio2_fun);
      symbol_table.add_function("xAxis", xAxis_fun);
      symbol_table.add_function("yAxis", yAxis_fun);
      symbol_table.add_function("xLog", xLog_fun);
      symbol_table.add_function("yLog", yLog_fun);
      symbol_table.add_function("xAxisPi", xAxisPi_fun);

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
    String<double> title_fun{title};
    std::string labelX;
    String<double> labelX_fun{labelX};
    std::string labelY;
    String<double> labelY_fun{labelY};
    std::string labelX2;
    String<double> labelX2_fun{labelX2};
    std::string labelY2;
    String<double> labelY2_fun{labelY2};
    std::optional<std::pair<double,double>> xlim;
    Lim<double> xlim_fun{xlim};
    std::optional<std::pair<double,double>> ylim;
    Lim<double> ylim_fun{ylim};
    std::optional<std::pair<double,double>> xlim2;
    Lim<double> xlim2_fun{xlim2};
    std::optional<std::pair<double,double>> ylim2;
    Lim<double> ylim2_fun{ylim2};
    std::optional<double> axisRatio;
    Double<double> axisRatio_fun{axisRatio};
    std::optional<double> axisRatio2;
    Double<double> axisRatio2_fun{axisRatio};
    std::optional<double> xAxis;
    Double<double> xAxis_fun{xAxis};
    std::optional<double> yAxis;
    Double<double> yAxis_fun{yAxis};
    std::optional<bool> xLog;
    Bool<double> xLog_fun{xLog};
    std::optional<bool> yLog;
    Bool<double> yLog_fun{yLog};
    std::optional<bool> xAxisPi;
    Bool<double> xAxisPi_fun{xAxisPi};
};

static bool ends_with(const std::string& str, const std::string& suffix)
{
  return str.size() >= suffix.size() && str.compare(str.size()-suffix.size(), suffix.size(), suffix) == 0;
}

void MainWindow::updateCalculation()
{
  Parser parser;

  auto parser_input = ui->plainTextEdit->toPlainText().toStdString();

  //bool found = false;
  //auto range = ui->widget->graph()->getValueRange(found);

  if (parser.parser.compile(parser_input, parser.expression))
  {
    parser.expression.value(); // needs to run at least once, to make xLog etc. available.
    auto range = ui->widget->xAxis->range();
    {
      auto rangeWidth = range.upper - range.lower;
      if (! (parser.xLog.has_value() && parser.xLog.value()) )
      {
        range.lower -= rangeWidth;
      }
      range.upper += rangeWidth;
    }
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
    double logUpper = log10(range.upper);
    double logLower = log10(range.lower);

    for (int i=0; i<N; ++i)
    {
      if (parser.xLog.has_value() && parser.xLog.value())
      {
        X[i] = pow(10.0, (logUpper - logLower) / (double)(N-1) * (double)i + logLower);
      }
      else
      {
        X[i] = i * (range.upper - range.lower) / (double)(N-1) + range.lower;
      }
      parser.x = X[i];
      expr_value = parser.expression.value();
      for (auto &v : variables)
      {
        v.Y[i] = *(v.value);
      }
    }
    bool showXAxis2 = false;
    bool showYAxis2 = false;
    for (auto &v : variables)
    {
      // create graph and assign data to it:
      QCPAxis * xAxis = nullptr;
      QCPAxis * yAxis = nullptr;
      QString name = v.name.c_str();
      if (std::regex_search(v.name, std::regex("__[12][12]$")))
      {
        name.chop(4);
        if ('2' == v.name[v.name.length()-2])
        {
          xAxis = customPlot->xAxis2;
          showXAxis2 = true;
        }
        if ('2' == v.name[v.name.length()-1])
        {
          yAxis = customPlot->yAxis2;
          showYAxis2 = true;
        }
      }
      name.replace("__", " ");
      customPlot->addGraph(xAxis, yAxis);
      auto graphIndex = customPlot->graphCount()-1;
      auto graph = customPlot->graph(graphIndex);
      graph->setAntialiased(true);
      //graph->setData(X, Y[v.first]);
      graph->setData(X, v.Y);
      graph->setName(name);
      graph->setPen(QPen(graphColors[graphIndex % graphColors.size()]));
    }
    customPlot->xAxis2->setVisible(showXAxis2);
    customPlot->yAxis2->setVisible(showYAxis2);

    customPlot->xAxis->setLabel("x");
    customPlot->yAxis->setLabel("y");
    if (ui->actionApply_limits_and_ratios->isChecked())
    {
      auto setLimits = [this](QCPAxis * axis, std::optional<std::pair<double,double>> & limits) {
        if (limits.has_value())
        {
          axis->setRange(limits.value().first, limits.value().second);
        }
      };
      setLimits(customPlot->xAxis, parser.xlim);
      setLimits(customPlot->yAxis, parser.ylim);
      setLimits(customPlot->xAxis2, parser.xlim2);
      setLimits(customPlot->yAxis2, parser.ylim2);
      if (parser.axisRatio.has_value())
      {
        customPlot->yAxis->setScaleRatio(customPlot->xAxis, parser.axisRatio.value());
      }
      if (parser.axisRatio2.has_value() && parser.axisRatio2.value())
      {
        customPlot->yAxis2->setScaleRatio(customPlot->xAxis2, parser.axisRatio2.value());
      }
    }
    auto setAxisLog = [this](QCPAxis * axis, std::optional<bool> & log) {
      if (log.has_value() && log.value())
      {
        QSharedPointer<QCPAxisTickerLog> logTicker(new QCPAxisTickerLog);
        axis->setScaleType(QCPAxis::stLogarithmic);
        axis->setTicker(logTicker);
        axis->setNumberFormat("eb"); // e = exponential, b = beautiful decimal powers
        axis->setNumberPrecision(0); // makes sure "1*10^4" is displayed only as "10^4"
      }
      else
      {
        QSharedPointer<QCPAxisTicker> linTicker(new QCPAxisTicker);
        axis->setScaleType(QCPAxis::stLinear);
        axis->setTicker(linTicker);
        axis->setNumberFormat("gb");
        axis->setNumberPrecision(6);
      }
    };
    setAxisLog(customPlot->xAxis, parser.xLog);
    setAxisLog(customPlot->yAxis, parser.yLog);
    if (parser.xAxisPi.has_value() && parser.xAxisPi.value())
    {
      QSharedPointer<QCPAxisTickerPi> piTicker(new QCPAxisTickerPi);
      customPlot->xAxis->setTicker(piTicker);
    }
    if (!parser.labelX.empty()) {
      customPlot->xAxis->setLabel(parser.labelX.c_str());
    }
    if (!parser.labelY.empty()) {
      customPlot->yAxis->setLabel(parser.labelY.c_str());
    }
    if (!parser.labelX2.empty()) {
      customPlot->xAxis2->setLabel(parser.labelX2.c_str());
    }
    if (!parser.labelY2.empty()) {
      customPlot->yAxis2->setLabel(parser.labelY2.c_str());
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
    if (QGuiApplication::keyboardModifiers() & Qt::AltModifier)
    {
      ui->widget->axisRect()->setRangeZoomAxes(ui->widget->xAxis2, ui->widget->yAxis2);
    }
    else
    {
      ui->widget->axisRect()->setRangeZoomAxes(ui->widget->xAxis, ui->widget->yAxis);
    }

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
  if (event->type() == QEvent::MouseButtonPress)
  {
    if (QGuiApplication::keyboardModifiers() & Qt::AltModifier)
    {
      ui->widget->axisRect()->setRangeDragAxes(ui->widget->xAxis2, ui->widget->yAxis2);
    }
    else
    {
      ui->widget->axisRect()->setRangeDragAxes(ui->widget->xAxis, ui->widget->yAxis);
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
