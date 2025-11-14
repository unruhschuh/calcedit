#include "calceditedit.h"

#include <QPainter>
#include <QPen>
#include <QTextBlock>
#include <QClipboard>
#include <QApplication>
#include <QScrollBar>

//CalcEditEdit::CalcEditEdit()
//{
//
//}

CalcEditEdit::CalcEditEdit(QWidget *parent)
  : QTextEdit(parent)
{
  connect(this, &CalcEditEdit::textChanged, [this](){
    bool mlBlock = false;
    for (auto block = document()->begin(); block != document()->end(); block = block.next())
    {
      if (block.text().startsWith("///") || block.text().startsWith("##"))
      {
        mlBlock = true;
      }
      if (!mlBlock)
      {
        block.blockFormat().setLineHeight(100,QTextBlockFormat::LineDistanceHeight);
      }
      if (block.next().text().isEmpty())
      {
        mlBlock = false;
      }
    }
  });
}

void CalcEditEdit::paintEvent(QPaintEvent *event)
{
  QPainter painter(viewport());

#if 0
  // Determine space needed for results
  auto fontMetrics = painter.fontMetrics();
  int resultWidth = 0;
  for (QTextBlock block = firstVisibleBlock(); block.isValid(); block = block.next())
  {
    auto blockNumber = block.blockNumber();
    if (blockNumber < m_results.size() && ! m_results[blockNumber].empty())
    {
      std::string result = m_results[blockNumber] + " = ";
      QRect actualRect;
      auto width = fontMetrics.horizontalAdvance(result.c_str());
      if (width > resultWidth) resultWidth = width;
    }
  }
  setViewportMargins(resultWidth,0,0,0);
#endif

  auto getBoundingRect = [this](const QTextBlock & block) {
      if (!block.isValid()) return QRectF();
      auto r = block.layout()->boundingRect();
      r.translate(block.layout()->position());
      r.translate(0, -verticalScrollBar()->value());
      return r;
  };

  bool mlBlock = false;
  for (auto block = document()->begin(); block != document()->end(); block = block.next())
  {
    if (block.text().startsWith("///") || block.text().startsWith("##"))
    {
      mlBlock = true;
    }

    if (mlBlock)
    {
      //auto r = block.layout()->boundingRect();
      //r.translate(block.layout()->position());
      //r.translate(0, -verticalScrollBar()->value());
      auto r = getBoundingRect(block);
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

#if 1
  {
    //qDebug() << "updateEvent";
    //QTextBlock block = firstVisibleBlock();
    QTextBlock block = document()->firstBlock();
    int blockNumber = block.blockNumber();
    //int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    //int bottom = top + qRound(blockBoundingRect(block).height());
    int top = getBoundingRect(block).y();
    int bottom = top + getBoundingRect(block).height();
    while (block.isValid() && top <= event->rect().bottom()) {
      if (block.isVisible() && bottom >= event->rect().top()) {
        auto line = block.layout()->lineAt(block.layout()->lineCount()-1);
        auto x = line.naturalTextWidth() + 5;
        QString number = QString::number(blockNumber + 1);
        painter.setPen(Qt::darkGray);
        if (blockNumber < m_results.size() && ! m_results[blockNumber].empty())
        {
          std::string result = " = " + m_results[blockNumber];
          painter.drawText(x, top, line.width(), painter.fontMetrics().height(), Qt::AlignLeft, result.c_str());
        }
      }

      block = block.next();
      top = bottom;
      //bottom = top + qRound(blockBoundingRect(block).height());
      bottom = top + qRound(getBoundingRect(block).height());
      ++blockNumber;
    }
  }
#endif

  QTextEdit::paintEvent(event);
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
    QTextEdit::keyPressEvent(event);
  }
  // The line might change during QTextEdit::keyPressEvent so we need to do this afterwards
  line = textCursor().blockNumber();
  if (line < m_results.size())
  {
    emit currentResult(m_results[line].c_str());
  }
}

void CalcEditEdit::setResults(std::vector<std::string> results)
{
  m_results = results;
  viewport()->update();
}
