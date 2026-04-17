#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QTableWidgetItem>

MainWindow::MainWindow(const QSet<QString>& tapeAlphabet,
                       const QSet<QString>& extraSymbols,
                       QWidget *parent)
    : QMainWindow(parent), m_stepDelayMs(500),
    m_tapeAlphabet(tapeAlphabet), m_extraSymbols(extraSymbols)
{
    setWindowTitle("Эмулятор машины Тьюринга");

    m_machine = new TuringMachine(this);
    m_machine->setAlphabets(tapeAlphabet, extraSymbols);

    m_tapeWidget = new TapeWidget(this);

    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // Tape widget
    mainLayout->addWidget(m_tapeWidget);

    // Program table
    m_programTable = new QTableWidget(0, 0);
    mainLayout->addWidget(m_programTable);

    // State buttons
    QHBoxLayout *stateButtonsLayout = new QHBoxLayout();
    m_addStateButton = new QPushButton("+ Состояние");
    m_removeStateButton = new QPushButton("- Состояние");
    stateButtonsLayout->addWidget(m_addStateButton);
    stateButtonsLayout->addWidget(m_removeStateButton);
    mainLayout->addLayout(stateButtonsLayout);

    // Input word
    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->addWidget(new QLabel("Входное слово:"));
    m_inputWordEdit = new QLineEdit();
    inputLayout->addWidget(m_inputWordEdit);
    m_setStringButton = new QPushButton("Задать строку");
    inputLayout->addWidget(m_setStringButton);
    mainLayout->addLayout(inputLayout);

    // Control buttons
    QHBoxLayout *controlLayout = new QHBoxLayout();
    m_runButton = new QPushButton("Запустить");
    m_runButton->setEnabled(false);
    m_stopButton = new QPushButton("Остановить");
    m_stopButton->setEnabled(false);
    m_stepButton = new QPushButton("Шаг");
    m_stepButton->setEnabled(false);
    m_resetButton = new QPushButton("Сбросить");
    m_resetButton->setEnabled(false);
    m_speedUpButton = new QPushButton("Ускорить");
    m_speedUpButton->setEnabled(false);
    m_slowDownButton = new QPushButton("Замедлить");
    m_slowDownButton->setEnabled(false);

    controlLayout->addWidget(m_runButton);
    controlLayout->addWidget(m_stopButton);
    controlLayout->addWidget(m_stepButton);
    controlLayout->addWidget(m_resetButton);
    controlLayout->addWidget(m_speedUpButton);
    controlLayout->addWidget(m_slowDownButton);
    mainLayout->addLayout(controlLayout);

    setCentralWidget(central);

    m_runTimer = new QTimer(this);
    m_runTimer->setInterval(m_stepDelayMs);
    connect(m_runTimer, &QTimer::timeout, this, &MainWindow::stepMachine);

    // Connections
    connect(m_setStringButton, &QPushButton::clicked, this, &MainWindow::setString);
    connect(m_runButton, &QPushButton::clicked, this, &MainWindow::runMachine);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::stopMachine);
    connect(m_stepButton, &QPushButton::clicked, this, &MainWindow::stepMachine);
    connect(m_resetButton, &QPushButton::clicked, this, &MainWindow::resetMachine);
    connect(m_speedUpButton, &QPushButton::clicked, this, &MainWindow::speedUp);
    connect(m_slowDownButton, &QPushButton::clicked, this, &MainWindow::slowDown);
    connect(m_addStateButton, &QPushButton::clicked, this, &MainWindow::addState);
    connect(m_removeStateButton, &QPushButton::clicked, this, &MainWindow::removeState);
    connect(m_programTable, &QTableWidget::cellChanged, this, &MainWindow::onCellChanged);
    connect(m_tapeWidget, &TapeWidget::animationFinished, this, &MainWindow::onTapeAnimationFinished);
    connect(m_machine, &TuringMachine::stateChanged, this, &MainWindow::updateTableHighlight);
    connect(m_machine, &TuringMachine::halted, this, [this]() {
        m_runTimer->stop();
        m_stopButton->setEnabled(false);
        m_stepButton->setEnabled(false);
        m_runButton->setEnabled(false);
        m_resetButton->setEnabled(true);
    });

    // Initialize states list
    m_statesList.append("q0");

    // Build initial table
    buildTable();
}

MainWindow::~MainWindow() {}

void MainWindow::buildTable()
{
    m_programTable->blockSignals(true);

    QStringList symbols;
    for (const QString& sym : m_tapeAlphabet) {
        symbols.append(sym);
    }
    for (const QString& sym : m_extraSymbols) {
        symbols.append(sym);
    }
    symbols.sort();

    m_programTable->setColumnCount(symbols.size());
    m_programTable->setHorizontalHeaderLabels(symbols);

    // Restore rows from states list
    m_programTable->setRowCount(m_statesList.size());
    for (int i = 0; i < m_statesList.size(); ++i) {
        m_programTable->setVerticalHeaderItem(i, new QTableWidgetItem(m_statesList[i]));
    }

    m_programTable->blockSignals(false);
}

void MainWindow::setString()
{
    QString word = m_inputWordEdit->text();

    // Validate
    for (QChar ch : word) {
        QString sym(ch);
        if (!m_tapeAlphabet.contains(sym) && !m_extraSymbols.contains(sym)) {
            QMessageBox::warning(this, "Ошибка",
                                 QString("Символ '%1' не входит в алфавит").arg(sym));
            return;
        }
    }

    // Create tape
    QVector<QString> tape;
    for (QChar ch : word) {
        tape.append(QString(ch));
    }
    if (tape.isEmpty()) {
        tape.append("Λ");
    }

    // Collect program
    QMap<QString, QMap<QString, Transition>> program = collectProgram();
    m_machine->setProgram(program);

    // Check for halt
    if (!m_machine->programHasHalt()) {
        QMessageBox::warning(this, "Ошибка",
                             "В программе нет команды остановки (состояние '!')");
        return;
    }

    m_machine->setInitialTape(tape);
    m_tapeWidget->setTape(m_machine->tape(), m_machine->headPosition());

    m_runButton->setEnabled(true);
    m_stepButton->setEnabled(true);
    m_resetButton->setEnabled(true);
    m_speedUpButton->setEnabled(true);
    m_slowDownButton->setEnabled(true);

    enableInputs(false);
}

QMap<QString, QMap<QString, Transition>> MainWindow::collectProgram()
{
    QMap<QString, QMap<QString, Transition>> program;

    for (int row = 0; row < m_programTable->rowCount(); ++row) {
        QString state = m_programTable->verticalHeaderItem(row)->text();
        QMap<QString, Transition> transitions;

        for (int col = 0; col < m_programTable->columnCount(); ++col) {
            QString symbol = m_programTable->horizontalHeaderItem(col)->text();
            QTableWidgetItem *item = m_programTable->item(row, col);

            if (item && !item->text().isEmpty()) {
                QString text = item->text();
                QStringList parts = text.split(' ', Qt::SkipEmptyParts);
                if (parts.size() >= 3) {
                    Transition t;
                    t.writeSymbol = parts[0];
                    t.direction = parts[1];
                    t.nextState = parts[2];
                    transitions[symbol] = t;
                }
            }
        }

        if (!transitions.isEmpty()) {
            program[state] = transitions;
        }
    }

    return program;
}

void MainWindow::runMachine()
{
    if (m_machine->isHalted()) return;
    m_runTimer->start();
    m_runButton->setEnabled(false);
    m_stopButton->setEnabled(true);
    m_stepButton->setEnabled(false);
}

void MainWindow::stopMachine()
{
    m_runTimer->stop();
    m_runButton->setEnabled(true);
    m_stopButton->setEnabled(false);
    m_stepButton->setEnabled(true);
}

void MainWindow::stepMachine()
{
    if (m_machine->isHalted()) {
        m_runTimer->stop();
        return;
    }

    m_machine->step();
    m_tapeWidget->setTape(m_machine->tape(), m_machine->headPosition());

    if (m_machine->isHalted()) {
        m_runTimer->stop();
        m_runButton->setEnabled(false);
        m_stopButton->setEnabled(false);
        m_stepButton->setEnabled(false);
    }
}

void MainWindow::resetMachine()
{
    stopMachine();
    m_machine->reset();

    // Re-initialize tape
    QString word = m_inputWordEdit->text();
    QVector<QString> tape;
    for (QChar ch : word) {
        tape.append(QString(ch));
    }
    if (tape.isEmpty()) {
        tape.append("Λ");
    }
    m_machine->setInitialTape(tape);
    m_tapeWidget->setTape(m_machine->tape(), m_machine->headPosition());

    enableInputs(false);
    m_runButton->setEnabled(true);
    m_stepButton->setEnabled(true);
}

void MainWindow::speedUp()
{
    m_stepDelayMs = qMax(50, m_stepDelayMs - 50);
    m_runTimer->setInterval(m_stepDelayMs);
    m_tapeWidget->setSpeed(m_stepDelayMs);
}

void MainWindow::slowDown()
{
    m_stepDelayMs += 50;
    m_runTimer->setInterval(m_stepDelayMs);
    m_tapeWidget->setSpeed(m_stepDelayMs);
}

QString MainWindow::generateNewStateName()
{
    int maxNum = -1;
    for (const QString& state : m_statesList) {
        if (state.startsWith('q')) {
            bool ok;
            int num = state.mid(1).toInt(&ok);
            if (ok && num > maxNum) {
                maxNum = num;
            }
        }
    }
    return QString("q%1").arg(maxNum + 1);
}

void MainWindow::addState()
{
    QString newState = generateNewStateName();
    m_statesList.append(newState);

    int row = m_programTable->rowCount();
    m_programTable->setRowCount(row + 1);
    m_programTable->setVerticalHeaderItem(row, new QTableWidgetItem(newState));
}

void MainWindow::removeState()
{
    if (m_programTable->rowCount() <= 1) {
        return; // Keep at least q0
    }

    int row = m_programTable->currentRow();
    if (row < 0) {
        row = m_programTable->rowCount() - 1;
    }

    QString stateToRemove = m_programTable->verticalHeaderItem(row)->text();
    if (stateToRemove == "q0") {
        QMessageBox::information(this, "Информация",
                                 "Нельзя удалить начальное состояние q0");
        return;
    }

    m_statesList.removeOne(stateToRemove);
    m_programTable->removeRow(row);
}

void MainWindow::onCellChanged(int row, int col)
{
    // Could add validation here
    Q_UNUSED(row)
    Q_UNUSED(col)
}

void MainWindow::onTapeAnimationFinished()
{
    if (m_runTimer->isActive() && !m_machine->isHalted()) {
        m_machine->step();
        m_tapeWidget->setTape(m_machine->tape(), m_machine->headPosition());
    }
}

void MainWindow::updateTableHighlight(const QString& state)
{
    for (int i = 0; i < m_programTable->rowCount(); ++i) {
        QTableWidgetItem *header = m_programTable->verticalHeaderItem(i);
        if (header && header->text() == state) {
            m_programTable->selectRow(i);
            break;
        }
    }
}

void MainWindow::enableInputs(bool enable)
{
    m_inputWordEdit->setEnabled(enable);
    m_setStringButton->setEnabled(enable);
    m_programTable->setEnabled(enable);
    m_addStateButton->setEnabled(enable);
    m_removeStateButton->setEnabled(enable);
}