#include "calceditedit.h"

#include <QPainter>
#include <QPen>
#include <QTextBlock>
#include <QClipboard>
#include <QApplication>
#include <QScrollBar>
#include <QSignalBlocker>

//CalcEditEdit::CalcEditEdit()
//{
//
//}

CalcEditEdit::CalcEditEdit(QWidget *parent)
  : QTextEdit(parent)
{
  connect(this, &CalcEditEdit::textChanged, [this](){
  });
  m_reformatTimer.start(1000);
  connect(&m_reformatTimer, &QTimer::timeout, [this](){
    //bool wasBlocked = blockSignals(true);
    const QSignalBlocker blocker{this->document()};
    bool mlBlock = false;
    for (auto block = document()->begin(); block != document()->end(); block = block.next())
    {
      if (block.isValid())
      {
        qDebug() << block.blockNumber();
        if (block.text().startsWith("///") || block.text().startsWith("##"))
        {
          mlBlock = true;
        }
        QTextCursor cursor(block);
        auto blockFormat = block.blockFormat();
        if (!mlBlock && block.blockNumber() < m_results.size() && !m_results[block.blockNumber()].empty() )
        {
          blockFormat.setBottomMargin(20);
        }
        else
        {
          blockFormat.setBottomMargin(0);
        }
        cursor.setBlockFormat(blockFormat);

        if (block.next().text().isEmpty())
        {
          mlBlock = false;
        }
      }
      else
      {
        qDebug() << "invalid block";
      }
    }
    //blockSignals(wasBlocked);
    update();
  });
}

void CalcEditEdit::paintEvent(QPaintEvent *event)
{
  QPainter painter(viewport());

  {
  }

#if 0
  // Determine space needed for results
  auto fontMetrics = painter.fontMetrics();
  int resultWidth = 0;
  for (QTextBlock block = document()->begin(); block.isValid(); block = block.next())
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
    //int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    //int bottom = top + qRound(blockBoundingRect(block).height());
    while (block.isValid()) {
      int blockNumber = block.blockNumber();
      int top = getBoundingRect(block).y();
      if (block.isVisible()) {
        auto line = block.layout()->lineAt(block.layout()->lineCount()-1);
        auto x = line.naturalTextWidth() + 5;
        QString number = QString::number(blockNumber + 1);
        painter.setPen(Qt::darkGray);
        if (blockNumber < m_results.size() && ! m_results[blockNumber].empty())
        {
          std::string result = " = " + m_results[blockNumber];
          //painter.drawText(x, top, line.width(), painter.fontMetrics().height(), Qt::AlignLeft, result.c_str());
          painter.drawText(0, getBoundingRect(block).y() + getBoundingRect(block).height(), line.width(), painter.fontMetrics().height(), Qt::AlignLeft, result.c_str());
        }
      }
      block = block.next();
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

  if (1)
  {
  }
}

void CalcEditEdit::setResults(std::vector<std::string> results)
{
  m_results = results;
  viewport()->update();
}
