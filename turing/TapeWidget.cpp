#include "TapeWidget.h"
#include "TuringMachine.h"
#include <QPainter>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QResizeEvent>
#include <QDebug>

TapeWidget::TapeWidget(QWidget *parent)
    : QWidget(parent), m_headPos(0), m_visibleStartIndex(0),
    m_targetVisibleStartIndex(0), m_headOffset(0.0), m_tapeOffset(0.0),
    m_cellWidth(70), m_cellHeight(70), m_visibleCells(11),
    m_isAnimating(false), m_animationInProgress(false)
{
    setMinimumSize(m_cellWidth * m_visibleCells, m_cellHeight * 2 + 20);

    m_headAnimation = new QPropertyAnimation(this, "headOffset", this);
    m_headAnimation->setDuration(400);
    m_headAnimation->setEasingCurve(QEasingCurve::InOutCubic);

    m_tapeAnimation = new QPropertyAnimation(this, "tapeOffset", this);
    m_tapeAnimation->setDuration(400);
    m_tapeAnimation->setEasingCurve(QEasingCurve::InOutCubic);

    m_animationGroup = new QParallelAnimationGroup(this);
    m_animationGroup->addAnimation(m_headAnimation);
    m_animationGroup->addAnimation(m_tapeAnimation);

    connect(m_animationGroup, &QParallelAnimationGroup::finished,
            this, &TapeWidget::onAnimationFinished);

    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::white);
    setPalette(pal);
}

void TapeWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateCellSize();
    update();
}

void TapeWidget::updateCellSize()
{
    int availableWidth = width() - 20;
    m_cellWidth = availableWidth / m_visibleCells;
    if (m_cellWidth < 50) m_cellWidth = 50;
    if (m_cellWidth > 100) m_cellWidth = 100;
    m_cellHeight = m_cellWidth;

    setMinimumHeight(m_cellHeight * 2 + 20);
}

void TapeWidget::setTape(const QVector<QString>& tape, int headPos)
{
    int oldHeadPos = m_headPos;
    m_tape = tape;
    m_headPos = headPos;

    // Если анимация уже идёт, останавливаем её
    if (m_animationGroup->state() == QAnimationGroup::Running) {
        m_animationGroup->stop();
        // После остановки свойства остаются в текущих значениях
    }

    // Определяем, нужно ли двигать ленту
    m_targetVisibleStartIndex = m_visibleStartIndex;

    // Проверяем, не вышел ли указатель за границы видимой области
    if (m_headPos < m_visibleStartIndex + 2) {
        m_targetVisibleStartIndex = qMax(0, m_headPos - m_visibleCells / 3);
    } else if (m_headPos >= m_visibleStartIndex + m_visibleCells - 2) {
        m_targetVisibleStartIndex = qMin(qMax(0, m_tape.size() - m_visibleCells),
                                         m_headPos - m_visibleCells * 2 / 3);
    }

    // Вычисляем смещения для анимации
    qreal startHeadOffset = m_headOffset;
    qreal startTapeOffset = m_tapeOffset;

    // Если головка двигается, добавляем смещение
    if (oldHeadPos != m_headPos) {
        // Направление движения: отрицательное смещение если двигаемся вправо
        qreal direction = (oldHeadPos < m_headPos) ? -1.0 : 1.0;
        startHeadOffset += direction;
    }

    // Если лента должна сдвинуться
    if (m_targetVisibleStartIndex != m_visibleStartIndex) {
        startTapeOffset += (m_visibleStartIndex - m_targetVisibleStartIndex);
    }

    // Устанавливаем начальные значения
    m_headOffset = startHeadOffset;
    m_tapeOffset = startTapeOffset;

    // Настраиваем анимацию к нулевым значениям
    m_headAnimation->setStartValue(m_headOffset);
    m_headAnimation->setEndValue(0.0);

    m_tapeAnimation->setStartValue(m_tapeOffset);
    m_tapeAnimation->setEndValue(0.0);

    m_animationInProgress = true;
    m_animationGroup->start();

    update();
}

void TapeWidget::setSpeed(int msPerStep)
{
    m_headAnimation->setDuration(msPerStep);
    m_tapeAnimation->setDuration(msPerStep);
}

QSize TapeWidget::sizeHint() const
{
    return QSize(m_cellWidth * m_visibleCells + 20, m_cellHeight * 2 + 20);
}

void TapeWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // Вычисляем текущее смещение ленты с учётом анимации
    qreal currentVisibleStart = m_visibleStartIndex + m_tapeOffset;
    int offsetX = 10 - currentVisibleStart * m_cellWidth;

    // Рисуем ячейки ленты
    for (int i = 0; i < m_tape.size(); ++i) {
        int x = offsetX + i * m_cellWidth;

        // Пропускаем невидимые ячейки
        if (x + m_cellWidth < 0 || x > width()) {
            continue;
        }

        QRect cellRect(x, m_cellHeight + 10, m_cellWidth, m_cellHeight);

        // Подсветка текущей ячейки (если не в процессе анимации)
        if (i == m_headPos && !m_animationInProgress) {
            painter.setPen(QPen(QColor(255, 200, 200), 2));
            painter.setBrush(QColor(255, 240, 240));
        } else {
            painter.setPen(QPen(Qt::gray, 1));
            painter.setBrush(Qt::white);
        }
        painter.drawRect(cellRect);

        // Рисуем символ
        QString symbol = m_tape.at(i);
        painter.setPen(QPen(Qt::black, 2));

        QFont font = painter.font();
        font.setPointSize(m_cellWidth / 3);
        font.setBold(true);
        painter.setFont(font);

        painter.drawText(cellRect, Qt::AlignCenter, symbol);
    }

    // Рисуем каретку с учётом смещения анимации
    qreal headDrawPos = m_headPos + m_headOffset;
    int headX = offsetX + headDrawPos * m_cellWidth + m_cellWidth / 2;
    int headY = m_cellHeight + 5;

    // Рисуем указатель
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(QColor(255, 80, 80)));

    QPolygon headPoly;
    headPoly << QPoint(headX, headY)
             << QPoint(headX - m_cellWidth / 4, headY - m_cellHeight / 5)
             << QPoint(headX + m_cellWidth / 4, headY - m_cellHeight / 5);
    painter.drawPolygon(headPoly);

    // Линия под кареткой
    painter.setPen(QPen(QColor(255, 80, 80), 2));
    painter.drawLine(headX, headY - m_cellHeight / 5,
                     headX, headY + m_cellHeight / 3);

    // Информация о позиции
    painter.setPen(QPen(Qt::darkGray, 1));
    QFont smallFont = painter.font();
    smallFont.setPointSize(8);
    painter.setFont(smallFont);

    QString posText = QString("Позиция: %1").arg(m_headPos);
    painter.drawText(10, 10, posText);
}

void TapeWidget::setHeadOffset(qreal offset)
{
    m_headOffset = offset;
    update();
}

void TapeWidget::setTapeOffset(qreal offset)
{
    m_tapeOffset = offset;
    update();
}

void TapeWidget::onAnimationFinished()
{
    m_animationInProgress = false;
    m_visibleStartIndex = m_targetVisibleStartIndex;
    m_headOffset = 0.0;
    m_tapeOffset = 0.0;
    update();
    emit animationFinished();
}

void TapeWidget::adjustVisibleRange()
{
    // Не используется
}

int TapeWidget::indexToX(int index) const
{
    return 10 + (index - m_visibleStartIndex) * m_cellWidth;
}