#include "savewavdialog.h"
#include "ui_savewavdialog.h"

SaveWavDialog::SaveWavDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::SaveWavDialog)
{
  ui->setupUi(this);

  connect(ui->ok, &QPushButton::clicked, this, &SaveWavDialog::accept);
  connect(ui->cancel, &QPushButton::clicked, this, &SaveWavDialog::reject);
}

SaveWavDialog::~SaveWavDialog()
{
  delete ui;
}

double SaveWavDialog::from()
{
  return ui->from->text().toDouble();
}
double SaveWavDialog::to()
{
  return ui->to->text().toDouble();
}
size_t SaveWavDialog::N()
{
  return ui->number->text().toULongLong();
}
size_t SaveWavDialog::loop()
{
  return ui->loop->text().toULongLong();
}
