#include "TuringMachine.h"
#include <QDebug>

TuringMachine::TuringMachine(QObject *parent)
    : QObject(parent), m_headPos(0), m_state("q0"), m_halted(true)
{
}

void TuringMachine::setAlphabets(const QSet<QString>& tapeAlphabet, const QSet<QString>& extraSymbols)
{
    m_tapeAlphabet = tapeAlphabet;
    m_extraSymbols = extraSymbols;
    m_states.clear();
    m_states.insert("q0");
    m_program.clear();
}

void TuringMachine::setProgram(const QMap<QString, QMap<QString, Transition>>& program)
{
    m_program = program;
    m_states.clear();
    m_states.insert("q0");
    for (auto it = program.begin(); it != program.end(); ++it) {
        m_states.insert(it.key());
        for (auto jt = it.value().begin(); jt != it.value().end(); ++jt) {
            m_states.insert(jt.value().nextState);
        }
    }
}

void TuringMachine::setInitialTape(const QVector<QString>& tape)
{
    m_tape = tape;
    if (m_tape.isEmpty()) {
        m_tape.append("Λ");
    }
    // Add empty symbols around
    m_tape.prepend("Λ");
    m_tape.prepend("Λ");
    m_tape.append("Λ");
    m_tape.append("Λ");
    m_headPos = 2; // start after the two leading empty symbols
    m_state = "q0";
    m_halted = false;
    emit tapeChanged();
    emit stateChanged(m_state);
}

void TuringMachine::reset()
{
    m_halted = true;
    m_state = "q0";
    emit stateChanged(m_state);
}

void TuringMachine::ensureTapeSize(int index)
{
    while (index < 0) {
        m_tape.prepend("Λ");
        m_headPos++;
        index++;
    }
    while (index >= m_tape.size()) {
        m_tape.append("Λ");
    }
}

bool TuringMachine::step()
{
    if (m_halted) return true;
    if (m_state == "!") {
        m_halted = true;
        emit halted();
        return true;
    }

    ensureTapeSize(m_headPos);
    QString currentSymbol = m_tape.at(m_headPos);

    if (!m_program.contains(m_state) || !m_program[m_state].contains(currentSymbol)) {
        m_halted = true;
        emit halted();
        return true;
    }

    Transition t = m_program[m_state][currentSymbol];
    m_tape[m_headPos] = t.writeSymbol;
    m_state = t.nextState;

    if (t.direction == "L") {
        m_headPos--;
        ensureTapeSize(m_headPos);
    } else if (t.direction == "R") {
        m_headPos++;
        ensureTapeSize(m_headPos);
    }

    emit tapeChanged();
    emit stateChanged(m_state);

    if (m_state == "!") {
        m_halted = true;
        emit halted();
        return true;
    }
    return false;
}

bool TuringMachine::programHasHalt() const
{
    for (auto it = m_program.begin(); it != m_program.end(); ++it) {
        for (auto jt = it.value().begin(); jt != it.value().end(); ++jt) {
            if (jt.value().nextState == "!") {
                return true;
            }
        }
    }
    return false;
}