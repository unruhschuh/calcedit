#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "calceditlib.h"


#include <QScrollBar>

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
  , ui(new Ui::MainWindow)
{
  ui->setupUi(this);

  connect(ui->input, &QPlainTextEdit::textChanged, this, &MainWindow::updateCalculation);

  ui->input->setPlainText(
    "// Comments start with double slash\n"
    "// Each non-empty line is an expression which is evaluated and the result\n"
    "// is displayed at the end of the same line.\n"
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
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::updateCalculation()
{
  std::string input = ui->input->toPlainText().toStdString();
  std::string results;
  std::map<std::string, double> variables;
  calculate(input, variables, results);
  ui->input->setResults(split_string_by_newline(results));

  QList<QTreeWidgetItem *> items;
  for (const auto & var : variables)
  {
    QStringList rows;
    rows.append(var.first.c_str());
    rows.append(QString::number(var.second));
    items.append(new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr), rows));
  }
  ui->treeWidget->clear();
  ui->treeWidget->insertTopLevelItems(0, items);
  ui->treeWidget->update();
  ui->input->update();
}
