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
    "var pi:=3.14159265359;\n"
    "if (x > 2*pi) {\n"
    "  x-2*pi;\n"
    "} else if (x > 0) {\n"
    "  sin(x);\n"
    "} else {\n"
    "  x;\n"
    "}\n");
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

void MainWindow::updateCalculation()
{
  double x = 0;
  symbol_table_t symbol_table;
  symbol_table.add_variable("x", x);
  expression_t expression;
  expression.register_symbol_table(symbol_table);
  parser_t parser;
  parser.settings().disable_commutative_check();
  auto parser_input = ui->plainTextEdit->toPlainText().toStdString();

  //bool found = false;
  //auto range = ui->widget->graph()->getValueRange(found);
  auto range = ui->widget->xAxis->range();

  if (parser.compile(parser_input, expression))
  {
    // generate some data:
    int N = 1001;
    QVector<double> X(N), Y(N); // initialize with entries 0..100
    for (int i=0; i<N; ++i)
    {
      X[i] = i * (range.upper - range.lower) / (double)(N-1) + range.lower;
      //X[i] = i/50.0 - 1; // x goes from -1 to 1
      x = X[i];
      Y[i] = expression.value();
    }
    auto customPlot = ui->widget;
    // create graph and assign data to it:
    customPlot->addGraph();
    customPlot->graph(0)->setData(X, Y);
    // give the axes some labels:
    customPlot->xAxis->setLabel("x");
    customPlot->yAxis->setLabel("y");
    // set axes ranges, so we see all data:
    //customPlot->xAxis->setRange(-1, 1);
    //customPlot->yAxis->setRange(0, 1);
    customPlot->replot();

  }
}
