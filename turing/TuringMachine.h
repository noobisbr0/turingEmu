#ifndef TURINGMACHINE_H
#define TURINGMACHINE_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QSet>

struct Transition {
    QString writeSymbol;
    QString direction; // "L", "R", "N"
    QString nextState;
};

class TuringMachine : public QObject
{
    Q_OBJECT
public:
    explicit TuringMachine(QObject *parent = nullptr);

    void setAlphabets(const QSet<QString>& tapeAlphabet, const QSet<QString>& extraSymbols);
    void setProgram(const QMap<QString, QMap<QString, Transition>>& program);
    void setInitialTape(const QVector<QString>& tape);
    void reset();

    bool step();

    QVector<QString> tape() const { return m_tape; }
    int headPosition() const { return m_headPos; }
    QString currentState() const { return m_state; }
    bool isHalted() const { return m_halted; }

    bool programHasHalt() const;

    QSet<QString> tapeAlphabet() const { return m_tapeAlphabet; }
    QSet<QString> states() const { return m_states; }
    QSet<QString> extraSymbols() const { return m_extraSymbols; }

signals:
    void tapeChanged();
    void stateChanged(const QString& state);
    void halted();

private:
    QSet<QString> m_tapeAlphabet;
    QSet<QString> m_extraSymbols;
    QSet<QString> m_states;
    QMap<QString, QMap<QString, Transition>> m_program;

    QVector<QString> m_tape;
    int m_headPos;
    QString m_state;
    bool m_halted;

    void ensureTapeSize(int index);
};

#endif // TURINGMACHINE_H