#ifndef SAVEWAVDIALOG_H
#define SAVEWAVDIALOG_H

#include <QDialog>

namespace Ui {
class SaveWavDialog;
}

class SaveWavDialog : public QDialog
{
    Q_OBJECT

  public:
    explicit SaveWavDialog(QWidget *parent = nullptr);
    ~SaveWavDialog();

    double from();
    double to();
    size_t N();
    size_t loop();

  private:
    Ui::SaveWavDialog *ui;
};

#endif // SAVEWAVDIALOG_H
