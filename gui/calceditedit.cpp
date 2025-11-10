#include "calceditedit.h"

#include <QPainter>
#include <QPen>
#include <QTextBlock>

//CalcEditEdit::CalcEditEdit()
//{
//
//}

CalcEditEdit::CalcEditEdit(QWidget *parent)
  : QPlainTextEdit(parent)
{

}

void CalcEditEdit::paintEvent(QPaintEvent *event)
{
  QPainter painter(viewport());

  //QRect r = this->geometry();
  //r.setTopLeft(QPoint(0,0));
  //QBrush b;
  //b.setColor(QColor(255,255,255));
  //painter.fillRect(r, b);

  //auto block = firstVisibleBlock();

#if 0
  for (auto block = document()->begin(); block != document()->end(); block = block.next())
  {


    auto r = blockBoundingGeometry(block);
    QBrush b;

    if (block.blockNumber() % 2)
    {
      b.setColor(QColor(200,200,200));
    }
    else
    {
      b.setColor(QColor(150, 150, 150));
    }
    b.setStyle(Qt::BrushStyle::SolidPattern);

    painter.fillRect(r, b);

    {
      QBrush b;
      b.setColor(Qt::white);
      QPen p;
      p.setColor(Qt::white);
      painter.setBrush(b);
      painter.setPen(p);
      auto line = block.layout()->lineAt(block.layout()->lineCount()-1);
      auto y = r.y() + r.height() + block.layout()->position().y() + line.y();
      auto x = line.naturalTextWidth();
      auto pos = QPoint(x, y);
      //pos.setX(line.width());
      painter.setFont(font());
      //painter.drawText(pos, "=5");
    }
  }

#endif
  {
    qDebug() << "updateEvent";
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());
    while (block.isValid() && top <= event->rect().bottom()) {
      if (block.isVisible() && bottom >= event->rect().top()) {
        auto line = block.layout()->lineAt(block.layout()->lineCount()-1);
        auto x = line.naturalTextWidth() + 5;
        QString number = QString::number(blockNumber + 1);
        painter.setPen(Qt::darkGray);
        std::string result;
        if (blockNumber < m_results.size() && ! m_results[blockNumber].empty())
        {
          result = " = " + m_results[blockNumber];
        }
        qDebug() << result;
        painter.drawText(x, top, line.width(), fontMetrics().height(),
             Qt::AlignLeft, result.c_str());
      }

      block = block.next();
      top = bottom;
      bottom = top + qRound(blockBoundingRect(block).height());
      ++blockNumber;
    }
  }


  QPlainTextEdit::paintEvent(event);
}

void CalcEditEdit::setResults(std::vector<std::string> results)
{
  m_results = results;
  viewport()->update();
}
