#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QScrollBar>

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

  connect(&m_recalcTimer, &QTimer::timeout, this, &MainWindow::updateCalculation);

  ui->widget->setInteraction(QCP::iRangeDrag, true);
  ui->widget->setInteraction(QCP::iRangeZoom, true);

  m_recalcTimer.setSingleShot(true);

  ui->plainTextEdit->setPlainText(
    "if (x > 2*pi) {\n"
    "  y:=x-2*pi;\n"
    "} else if (x > 0) {\n"
    "  y:=sin(x);\n"
    "} else {\n"
    "  y:=x;\n"
    "};\n"
    "if (x > -1 and x < 1)\n"
    "{\n"
    "  z:=1;\n"
    "} else {\n"
    "  z:=inf\n"
    "};\n"
    "a:=x^2;");
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

void MainWindow::updateCalculation()
{
  double x = 0;

  symbol_table_t unknown_var_symbol_table;

  symbol_table_t symbol_table;
  symbol_table.add_variable("x", x);
  symbol_table.add_constants();

  expression_t expression;
  expression.register_symbol_table(unknown_var_symbol_table);
  expression.register_symbol_table(symbol_table);

  parser_t parser;
  parser.enable_unknown_symbol_resolver();
  parser.settings().disable_commutative_check();
  auto parser_input = ui->plainTextEdit->toPlainText().toStdString();

  //bool found = false;
  //auto range = ui->widget->graph()->getValueRange(found);
  auto range = ui->widget->xAxis->range();

  if (parser.compile(parser_input, expression))
  {
    std::vector<std::string> variable_list;
    unknown_var_symbol_table.get_variable_list(variable_list);

    auto customPlot = ui->widget;
    customPlot->clearGraphs();
    for (auto v : variable_list)
    {
      qDebug() << v;
      double & y = unknown_var_symbol_table.variable_ref(v);
      // generate some data:
      int N = 1001;
      QVector<double> X(N), Y(N); // initialize with entries 0..100
      for (int i=0; i<N; ++i)
      {
        X[i] = i * (range.upper - range.lower) / (double)(N-1) + range.lower;
        //X[i] = i/50.0 - 1; // x goes from -1 to 1
        x = X[i];
        expression.value();
        Y[i] = y;
      }
      // create graph and assign data to it:
      customPlot->addGraph();
      auto graphIndex = customPlot->graphCount()-1;
      auto graph = customPlot->graph(graphIndex);
      graph->setData(X, Y);
      graph->setName(v.c_str());
      graph->setPen(QPen(graphColors[graphIndex % graphColors.size()]));
      // give the axes some labels:
      // set axes ranges, so we see all data:
      //customPlot->xAxis->setRange(-1, 1);
      //customPlot->yAxis->setRange(0, 1);
    }
    customPlot->xAxis->setLabel("x");
    customPlot->yAxis->setLabel("y");
    customPlot->legend->setVisible(true);
    customPlot->replot();
    statusBar()->clearMessage();
  }
  else if ( ! parser.lexer().empty() )
  {
    //statusBar()->showMessage(parser.error().c_str());
    typedef exprtk::parser_error::type error_t;

    error_t error = parser.get_error(0);

    exprtk::parser_error::update_error(error,parser_input);

    int i = 0;
    QString error_message = QString("Error[%1] at line: %2 column: %3").arg(i).arg(error.line_no).arg(error.column_no);
    statusBar()->showMessage(error_message);

  }
}
