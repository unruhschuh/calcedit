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
  ui->widget->xAxis->setRange(-0.5, 3.5);
  ui->widget->yAxis->setRange(-3.5, 3.5);

  ui->widget->installEventFilter(this);
  ui->widget->setNoAntialiasingOnDrag(true);

  m_recalcTimer.setSingleShot(true);

  ui->plainTextEdit->setPlainText(
    "// Comments start with // or #\n"
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
  {
    auto rangeWidth = range.upper - range.lower;
    range.lower -= rangeWidth;
    range.upper += rangeWidth;
  }

  if (parser.compile(parser_input, expression))
  {
    std::vector<std::string> variable_list;
    unknown_var_symbol_table.get_variable_list(variable_list);

    auto customPlot = ui->widget;
    customPlot->clearGraphs();
    for (const auto & v : variable_list)
    {
      double & y = unknown_var_symbol_table.variable_ref(v);
      // generate some data:
      int N = std::max(ui->widget->width()*2, 100);
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
      graph->setAntialiased(true);
      graph->setData(X, Y);
      QString name = v.c_str();
      name.replace('_', ' ');
      graph->setName(name);
      graph->setPen(QPen(graphColors[graphIndex % graphColors.size()]));
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
