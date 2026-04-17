#include "TuringMachine.h"

const QString TuringMachine::EMPTY_SYMBOL = "/\\";

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
    m_states.insert("!");
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
        m_tape.append(EMPTY_SYMBOL);
    }
    // Add empty symbols around
    m_tape.prepend(EMPTY_SYMBOL);
    m_tape.prepend(EMPTY_SYMBOL);
    m_tape.append(EMPTY_SYMBOL);
    m_tape.append(EMPTY_SYMBOL);
    m_headPos = 2;
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
        m_tape.prepend(EMPTY_SYMBOL);
        m_headPos++;
        index++;
    }
    while (index >= m_tape.size()) {
        m_tape.append(EMPTY_SYMBOL);
    }
}

Transition TuringMachine::parseCommand(const QString& command, const QString& currentSymbol, const QString& currentState)
{
    Transition t;
    t.writeSymbol = currentSymbol;
    t.direction = "N";
    t.nextState = currentState;

    QString text = command.trimmed();
    if (text.isEmpty()) {
        return t;
    }

    QStringList parts = text.split(' ', Qt::SkipEmptyParts);

    if (parts.size() == 1) {
        QString part = parts[0];
        if (part == "!") {
            t.nextState = "!";
        } else if (part == "L" || part == "R" || part == "N") {
            t.direction = part;
        } else {
            t.writeSymbol = part;
        }
    } else if (parts.size() == 2) {
        QString part1 = parts[0];
        QString part2 = parts[1];

        if (part2 == "!") {
            if (part1 == "L" || part1 == "R" || part1 == "N") {
                t.direction = part1;
                t.nextState = "!";
            } else {
                t.writeSymbol = part1;
                t.nextState = "!";
            }
        } else if (part2 == "L" || part2 == "R" || part2 == "N") {
            t.writeSymbol = part1;
            t.direction = part2;
        } else if (part1 == "L" || part1 == "R" || part1 == "N") {
            t.direction = part1;
            t.nextState = part2;
        } else {
            t.writeSymbol = part1;
            t.nextState = part2;
            t.direction = "N";
        }
    } else if (parts.size() >= 3) {
        QString part1 = parts[0];
        QString part2 = parts[1];
        QString part3 = parts[2];

        if (part2 == "L" || part2 == "R" || part2 == "N") {
            t.writeSymbol = part1;
            t.direction = part2;
            t.nextState = part3;
        } else {
            t.writeSymbol = part1;
            t.direction = "N";
            t.nextState = part3;
        }
    }

    return t;
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
        emit error(QString("Нет команды для состояния '%1' и символа '%2'").arg(m_state).arg(currentSymbol));
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