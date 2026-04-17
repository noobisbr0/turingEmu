#include "TapeWidget.h"
#include <QPainter>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QDebug>

TapeWidget::TapeWidget(QWidget *parent)
    : QWidget(parent), m_headPos(0), m_visibleStartIndex(0), m_headOffset(0.0),
      m_cellWidth(60), m_cellHeight(60), m_visibleCells(15)
{
    setMinimumSize(m_cellWidth * m_visibleCells, m_cellHeight * 2);
    m_headAnimation = new QPropertyAnimation(this, "headOffset", this);
    m_headAnimation->setDuration(500);
    m_headAnimation->setEasingCurve(QEasingCurve::InOutQuad);
    connect(m_headAnimation, &QPropertyAnimation::finished, this, &TapeWidget::onAnimationFinished);
}

void TapeWidget::setTape(const QVector<QString>& tape, int headPos)
{
    m_tape = tape;
    m_headPos = headPos;
    adjustVisibleRange();
    update();
}

void TapeWidget::setSpeed(int msPerStep)
{
    m_headAnimation->setDuration(msPerStep);
}

QSize TapeWidget::sizeHint() const
{
    return QSize(m_cellWidth * m_visibleCells, m_cellHeight * 2);
}

void TapeWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw cells
    for (int i = 0; i < m_visibleCells; ++i) {
        int tapeIndex = m_visibleStartIndex + i;
        QRect rect(i * m_cellWidth, m_cellHeight, m_cellWidth, m_cellHeight);
        painter.drawRect(rect);
        if (tapeIndex >= 0 && tapeIndex < m_tape.size()) {
            painter.drawText(rect, Qt::AlignCenter, m_tape.at(tapeIndex));
        }
    }

    // Draw head
    qreal headX = (m_headPos - m_visibleStartIndex + m_headOffset) * m_cellWidth;
    QPolygon headPoly;
    headPoly << QPoint(headX + m_cellWidth/2, m_cellHeight - 5)
             << QPoint(headX + m_cellWidth/2 - 10, m_cellHeight - 20)
             << QPoint(headX + m_cellWidth/2 + 10, m_cellHeight - 20);
    painter.setBrush(Qt::red);
    painter.drawPolygon(headPoly);
}

void TapeWidget::setHeadOffset(qreal offset)
{
    m_headOffset = offset;
    update();
}

void TapeWidget::onAnimationFinished()
{
    m_headOffset = 0.0;
    adjustVisibleRange();
    emit animationFinished();
}

void TapeWidget::adjustVisibleRange()
{
    // Keep head visible
    if (m_headPos < m_visibleStartIndex + 2) {
        m_visibleStartIndex = qMax(0, m_headPos - m_visibleCells / 4);
    } else if (m_headPos >= m_visibleStartIndex + m_visibleCells - 2) {
        m_visibleStartIndex = qMin(m_tape.size() - m_visibleCells, m_headPos - m_visibleCells * 3 / 4);
    }
    if (m_visibleStartIndex < 0) m_visibleStartIndex = 0;
    update();
}

int TapeWidget::indexToX(int index) const
{
    return (index - m_visibleStartIndex) * m_cellWidth;
}
