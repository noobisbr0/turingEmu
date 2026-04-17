#include "TapeWidget.h"
#include "TuringMachine.h"
#include <QPainter>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QTimer>

TapeWidget::TapeWidget(QWidget *parent)
    : QWidget(parent), m_headPos(0), m_oldHeadPos(0),
    m_scrollOffset(0.0), m_headScreenPos(5),
    m_cellWidth(70), m_cellHeight(70), m_visibleCells(11),
    m_isAnimating(false)
{
    setMinimumSize(m_cellWidth * m_visibleCells, m_cellHeight * 2 + 20);

    m_scrollAnimation = new QPropertyAnimation(this, "scrollOffset", this);
    m_scrollAnimation->setDuration(400);
    m_scrollAnimation->setEasingCurve(QEasingCurve::InOutCubic);

    connect(m_scrollAnimation, &QPropertyAnimation::finished,
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
    m_headScreenPos = m_visibleCells / 2;

    setMinimumHeight(m_cellHeight * 2 + 20);
}

void TapeWidget::setTape(const QVector<QString>& tape, int headPos)
{
    TapeState newState;
    newState.tape = tape;
    newState.headPos = headPos;

    // Если это первый запуск
    if (m_tape.isEmpty()) {
        m_tape = tape;
        m_headPos = headPos;
        m_oldHeadPos = headPos;
        m_currentState = newState;
        m_targetState = newState;
        update();
        return;
    }

    // Добавляем в очередь
    m_pendingStates.enqueue(newState);

    // Если анимация не идёт, запускаем следующий шаг
    if (!m_isAnimating && m_pendingStates.size() == 1) {
        processNextStep();
    }
}

void TapeWidget::processNextStep()
{
    if (m_pendingStates.isEmpty()) {
        return;
    }

    // Берём следующее состояние из очереди
    m_targetState = m_pendingStates.dequeue();

    // Запоминаем старую позицию для анимации
    m_oldHeadPos = m_headPos;

    // Обновляем ленту и позицию
    m_tape = m_targetState.tape;
    m_headPos = m_targetState.headPos;

    // Если позиция не изменилась - сразу завершаем шаг
    if (m_oldHeadPos == m_headPos) {
        m_currentState = m_targetState;
        update();
        emit stepCompleted();
        // Рекурсивно вызываем для обработки следующего шага
        QTimer::singleShot(0, this, &TapeWidget::processNextStep);
        return;
    }

    // Запускаем анимацию
    m_isAnimating = true;
    m_scrollOffset = 0.0;

    qreal endOffset = 0.0;
    if (m_headPos > m_oldHeadPos) {
        endOffset = -1.0;  // движение вправо
    } else {
        endOffset = 1.0;   // движение влево
    }

    m_scrollAnimation->setStartValue(0.0);
    m_scrollAnimation->setEndValue(endOffset);
    m_scrollAnimation->start();

    update();
}

void TapeWidget::setSpeed(int msPerStep)
{
    m_scrollAnimation->setDuration(msPerStep);
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

    if (m_tape.isEmpty()) {
        // Рисуем пустую ленту
        for (int i = 0; i < m_visibleCells; ++i) {
            int x = 10 + i * m_cellWidth;
            QRect cellRect(x, m_cellHeight + 10, m_cellWidth, m_cellHeight);
            painter.setPen(QPen(Qt::gray, 1));
            painter.setBrush(Qt::white);
            painter.drawRect(cellRect);
        }

        // Рисуем каретку
        int headX = 10 + m_headScreenPos * m_cellWidth + m_cellWidth / 2;
        int headY = m_cellHeight + 5;

        painter.setPen(Qt::NoPen);
        painter.setBrush(QBrush(QColor(255, 80, 80)));

        QPolygon headPoly;
        headPoly << QPoint(headX, headY)
                 << QPoint(headX - m_cellWidth / 4, headY - m_cellHeight / 5)
                 << QPoint(headX + m_cellWidth / 4, headY - m_cellHeight / 5);
        painter.drawPolygon(headPoly);

        return;
    }

    int headScreenX = 10 + m_headScreenPos * m_cellWidth;

    // Во время анимации показываем старую позицию как текущую
    int displayHeadPos = m_isAnimating ? m_oldHeadPos : m_headPos;

    // Рисуем ячейки
    for (int i = 0; i < m_tape.size(); ++i) {
        qreal cellOffset = i - displayHeadPos + m_scrollOffset;
        int x = headScreenX + cellOffset * m_cellWidth;

        if (x + m_cellWidth < 0 || x > width()) {
            continue;
        }

        QRect cellRect(x, m_cellHeight + 10, m_cellWidth, m_cellHeight);

        bool isCurrentCell = (i == displayHeadPos);

        if (isCurrentCell) {
            painter.setPen(QPen(QColor(255, 150, 150), 3));
            painter.setBrush(QColor(255, 240, 240));
        } else {
            painter.setPen(QPen(Qt::gray, 1));
            painter.setBrush(Qt::white);
        }
        painter.drawRect(cellRect);

        QString symbol = m_tape.at(i);
        painter.setPen(QPen(Qt::black, 2));

        QFont font = painter.font();
        font.setPointSize(m_cellWidth / 3);
        font.setBold(true);
        painter.setFont(font);

        painter.drawText(cellRect, Qt::AlignCenter, symbol);
    }

    // Рисуем каретку
    int headX = headScreenX + m_cellWidth / 2;
    int headY = m_cellHeight + 5;

    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(QColor(255, 80, 80)));

    QPolygon headPoly;
    headPoly << QPoint(headX, headY)
             << QPoint(headX - m_cellWidth / 4, headY - m_cellHeight / 5)
             << QPoint(headX + m_cellWidth / 4, headY - m_cellHeight / 5);
    painter.drawPolygon(headPoly);

    painter.setPen(QPen(QColor(255, 80, 80), 2));
    painter.drawLine(headX, headY - m_cellHeight / 5,
                     headX, headY + m_cellHeight / 3);

    // Информация
    painter.setPen(QPen(Qt::darkGray, 1));
    QFont smallFont = painter.font();
    smallFont.setPointSize(9);
    painter.setFont(smallFont);

    QString symbol = (m_headPos >= 0 && m_headPos < m_tape.size()) ? m_tape.at(m_headPos) : "?";
    QString posText = QString("Позиция: %1 | Символ: %2 | В очереди: %3")
                          .arg(m_headPos)
                          .arg(symbol)
                          .arg(m_pendingStates.size());
    painter.drawText(10, 20, posText);
}

void TapeWidget::setScrollOffset(qreal offset)
{
    m_scrollOffset = offset;
    update();
}

void TapeWidget::onAnimationFinished()
{
    m_isAnimating = false;
    m_scrollOffset = 0.0;
    m_currentState = m_targetState;

    update();
    emit stepCompleted();
    emit animationFinished();

    // Запускаем следующий шаг из очереди
    processNextStep();
}