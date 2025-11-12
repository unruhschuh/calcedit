#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

  private:
    Ui::MainWindow *ui;

  protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

  public slots:
    void updateCalculation();
    void startCalculationTimer();
    void saveWav();

  private:
    QTimer m_recalcTimer;
};
#endif // MAINWINDOW_H
