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
    m_cellWidth(70), m_cellHeight(70), m_visibleCells(11)
{
    setMinimumSize(m_cellWidth * m_visibleCells, m_cellHeight * 2 + 20);

    // Создаём анимации
    m_headAnimation = new QPropertyAnimation(this, "headOffset", this);
    m_headAnimation->setDuration(400);
    m_headAnimation->setEasingCurve(QEasingCurve::InOutCubic);

    m_tapeAnimation = new QPropertyAnimation(this, "tapeOffset", this);
    m_tapeAnimation->setDuration(400);
    m_tapeAnimation->setEasingCurve(QEasingCurve::InOutCubic);

    m_animationGroup = new QParallelAnimationGroup(this);
    m_animationGroup->addAnimation(m_headAnimation);
    m_animationGroup->addAnimation(m_tapeAnimation);

    connect(m_headAnimation, &QPropertyAnimation::finished,
            this, &TapeWidget::onHeadAnimationFinished);
    connect(m_tapeAnimation, &QPropertyAnimation::finished,
            this, &TapeWidget::onTapeAnimationFinished);

    // Устанавливаем белый фон
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

    // Останавливаем текущую анимацию
    m_animationGroup->stop();

    // Определяем, нужно ли двигать ленту
    m_targetVisibleStartIndex = m_visibleStartIndex;

    // Проверяем, не вышел ли указатель за границы видимой области
    if (m_headPos < m_visibleStartIndex + 2) {
        m_targetVisibleStartIndex = qMax(0, m_headPos - m_visibleCells / 3);
    } else if (m_headPos >= m_visibleStartIndex + m_visibleCells - 2) {
        m_targetVisibleStartIndex = qMin(qMax(0, m_tape.size() - m_visibleCells),
                                         m_headPos - m_visibleCells * 2 / 3);
    }

    // Настраиваем анимацию движения каретки
    if (oldHeadPos != headPos) {
        // Анимация перемещения каретки на одну ячейку
        m_headOffset = (oldHeadPos < headPos) ? -1.0 : 1.0;
        m_headAnimation->setStartValue(m_headOffset);
        m_headAnimation->setEndValue(0.0);
    } else {
        m_headOffset = 0.0;
        m_headAnimation->setStartValue(0.0);
        m_headAnimation->setEndValue(0.0);
    }

    // Настраиваем анимацию движения ленты
    if (m_targetVisibleStartIndex != m_visibleStartIndex) {
        m_tapeOffset = (m_visibleStartIndex - m_targetVisibleStartIndex);
        m_tapeAnimation->setStartValue(m_tapeOffset);
        m_tapeAnimation->setEndValue(0.0);
    } else {
        m_tapeOffset = 0.0;
        m_tapeAnimation->setStartValue(0.0);
        m_tapeAnimation->setEndValue(0.0);
    }

    // Запускаем анимацию
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

    // Вычисляем смещение для плавного скролла
    int offsetX = 10 - (m_visibleStartIndex + m_tapeOffset) * m_cellWidth;

    // Рисуем ячейки ленты
    for (int i = 0; i < m_tape.size(); ++i) {
        int x = offsetX + i * m_cellWidth;

        // Рисуем только видимые ячейки
        if (x + m_cellWidth < 0 || x > width()) {
            continue;
        }

        QRect cellRect(x, m_cellHeight + 10, m_cellWidth, m_cellHeight);

        // Рисуем границу ячейки
        painter.setPen(QPen(Qt::gray, 1));
        painter.setBrush(Qt::white);
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

    // Рисуем каретку (головку)
    int headX = offsetX + (m_headPos + m_headOffset) * m_cellWidth + m_cellWidth / 2;
    int headY = m_cellHeight + 5;

    // Рисуем указатель (треугольник)
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(QColor(255, 80, 80)));

    QPolygon headPoly;
    headPoly << QPoint(headX, headY)
             << QPoint(headX - m_cellWidth / 4, headY - m_cellHeight / 5)
             << QPoint(headX + m_cellWidth / 4, headY - m_cellHeight / 5);
    painter.drawPolygon(headPoly);

    // Рисуем линию под кареткой
    painter.setPen(QPen(QColor(255, 80, 80), 2));
    painter.drawLine(headX, headY - m_cellHeight / 5,
                     headX, headY + m_cellHeight / 3);

    // Рисуем информацию о текущей позиции
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

void TapeWidget::onHeadAnimationFinished()
{
    m_headOffset = 0.0;
}

void TapeWidget::onTapeAnimationFinished()
{
    m_tapeOffset = 0.0;
    m_visibleStartIndex = m_targetVisibleStartIndex;
    emit animationFinished();
}

void TapeWidget::adjustVisibleRange()
{
    // Устаревший метод, оставлен для совместимости
    // Логика перемещена в setTape
}

int TapeWidget::indexToX(int index) const
{
    return 10 + (index - m_visibleStartIndex) * m_cellWidth;
}