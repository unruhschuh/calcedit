#ifndef CALCEDITEDIT_H
#define CALCEDITEDIT_H

#include <QTextEdit>
#include <QTimer>

class CalcEditEdit : public QTextEdit
{
    Q_OBJECT
  public:
    //CalcEditEdit();
    CalcEditEdit(QWidget *parent = nullptr);

    void setResults(std::vector<std::string> results);

  protected:
    virtual void paintEvent(QPaintEvent *event) override;
    virtual void keyPressEvent(QKeyEvent *event) override;

  private:
    std::vector<std::string> m_results;

  signals:
    void currentResult(QString result);

  private:
    QTimer m_reformatTimer;
};

#endif // CALCEDITEDIT_H
