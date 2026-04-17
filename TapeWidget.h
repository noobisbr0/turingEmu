#ifndef TAPEWIDGET_H
#define TAPEWIDGET_H

#include <QWidget>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QVector>
#include <QString>

class TapeWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal headOffset READ headOffset WRITE setHeadOffset)
public:
    explicit TapeWidget(QWidget *parent = nullptr);

    void setTape(const QVector<QString>& tape, int headPos);
    void setSpeed(int msPerStep);

    QSize sizeHint() const override;

signals:
    void animationFinished();

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onAnimationFinished();

private:
    QVector<QString> m_tape;
    int m_headPos;
    int m_visibleStartIndex;
    qreal m_headOffset;
    QPropertyAnimation *m_headAnimation;
    int m_cellWidth;
    int m_cellHeight;
    int m_visibleCells;

    qreal headOffset() const { return m_headOffset; }
    void setHeadOffset(qreal offset);

    void adjustVisibleRange();
    int indexToX(int index) const;
};

#endif // TAPEWIDGET_H
