#ifndef TAPEWIDGET_H
#define TAPEWIDGET_H

#include <QWidget>
#include <QPropertyAnimation>
#include <QVector>
#include <QString>
#include <QEasingCurve>
#include <QQueue>

class TapeWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal scrollOffset READ scrollOffset WRITE setScrollOffset)

public:
    explicit TapeWidget(QWidget *parent = nullptr);

    void setTape(const QVector<QString>& tape, int headPos);
    void setSpeed(int msPerStep);
    void clearPendingStates();  // новый метод

    QSize sizeHint() const override;

signals:
    void animationFinished();
    void stepCompleted();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onAnimationFinished();
    void processNextStep();

private:
    struct TapeState {
        QVector<QString> tape;
        int headPos;
    };

    QVector<QString> m_tape;
    int m_headPos;
    int m_oldHeadPos;

    qreal m_scrollOffset;
    int m_headScreenPos;

    QPropertyAnimation *m_scrollAnimation;

    int m_cellWidth;
    int m_cellHeight;
    int m_visibleCells;

    bool m_isAnimating;

    QQueue<TapeState> m_pendingStates;
    TapeState m_currentState;
    TapeState m_targetState;

    qreal scrollOffset() const { return m_scrollOffset; }
    void setScrollOffset(qreal offset);

    void updateCellSize();
    void startAnimationTo(const TapeState& target);
};

#endif // TAPEWIDGET_H