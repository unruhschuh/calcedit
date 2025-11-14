#include "calceditedit.h"

#include <QPainter>
#include <QPen>
#include <QTextBlock>
#include <QClipboard>
#include <QApplication>

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

  bool mlBlock = false;
  for (auto block = document()->begin(); block != document()->end(); block = block.next())
  {
    if (block.text().startsWith("///") || block.text().startsWith("##"))
    {
      mlBlock = true;
    }

    if (mlBlock)
    {
      auto r = blockBoundingGeometry(block).translated(contentOffset());
      QBrush b;
      b.setColor(QColor(220,220,220));
      b.setStyle(Qt::BrushStyle::SolidPattern);
      painter.fillRect(r, b);
    }

    if (block.next().text().isEmpty())
    {
      mlBlock = false;
    }
  }

  {
    //qDebug() << "updateEvent";
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
        if (blockNumber < m_results.size() && ! m_results[blockNumber].empty())
        {
          std::string result = " = " + m_results[blockNumber];
          auto textRect = fontMetrics().boundingRect(result.c_str());
          if (block.next().text().trimmed().isEmpty() &&
              block.next() != textCursor().block() &&
              textRect.width() + x > line.width()
              )
          {
            x = line.width() - textRect.width();
            top = bottom;
          }
          //painter.drawText(x, top, line.width(), fontMetrics().height(), Qt::AlignLeft, result.c_str());
          painter.drawText(x, top, textRect.width(), fontMetrics().height(), Qt::AlignLeft, result.c_str());
        }
      }

      block = block.next();
      top = bottom;
      bottom = top + qRound(blockBoundingRect(block).height());
      ++blockNumber;
    }
  }


  QPlainTextEdit::paintEvent(event);
}

void CalcEditEdit::keyPressEvent(QKeyEvent *event)
{
  auto line = textCursor().blockNumber();
  if(
     event->type() == QKeyEvent::KeyPress &&
     event->matches(QKeySequence::Copy) &&
     textCursor().selectedText().isEmpty() &&
     line < m_results.size() )
  {
    QApplication::clipboard()->setText(m_results[line].c_str());
    event->accept();
  }
  else
  {
    QPlainTextEdit::keyPressEvent(event);
  }
  // The line might change during QPlainTextEdit::keyPressEvent so we need to do this afterwards
  line = textCursor().blockNumber();
  if (line < m_results.size())
  {
    emit currentResult(m_results[line].c_str());
  }
  viewport()->update();
}

void CalcEditEdit::setResults(std::vector<std::string> results)
{
  m_results = results;
  viewport()->update();
}
