#ifndef TAPEWIDGET_H
#define TAPEWIDGET_H

#include <QWidget>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QVector>
#include <QString>
#include <QEasingCurve>

class TapeWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal headOffset READ headOffset WRITE setHeadOffset)
    Q_PROPERTY(qreal tapeOffset READ tapeOffset WRITE setTapeOffset)

public:
    explicit TapeWidget(QWidget *parent = nullptr);

    void setTape(const QVector<QString>& tape, int headPos);
    void setSpeed(int msPerStep);

    QSize sizeHint() const override;

signals:
    void animationFinished();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onAnimationFinished();

private:
    QVector<QString> m_tape;
    int m_headPos;
    int m_visibleStartIndex;
    int m_targetVisibleStartIndex;

    qreal m_headOffset;
    qreal m_tapeOffset;

    QPropertyAnimation *m_headAnimation;
    QPropertyAnimation *m_tapeAnimation;
    QParallelAnimationGroup *m_animationGroup;

    int m_cellWidth;
    int m_cellHeight;
    int m_visibleCells;

    bool m_isAnimating;
    bool m_animationInProgress;

    qreal headOffset() const { return m_headOffset; }
    void setHeadOffset(qreal offset);

    qreal tapeOffset() const { return m_tapeOffset; }
    void setTapeOffset(qreal offset);

    void adjustVisibleRange();
    void updateCellSize();
    int indexToX(int index) const;
};

#endif // TAPEWIDGET_H