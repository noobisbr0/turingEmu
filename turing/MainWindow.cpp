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

    // Алфавит строки
    QStringList tapeSymbols = m_tapeAlphabet.values();
    tapeSymbols.sort();
    symbols.append(tapeSymbols);

    // Пустой символ
    symbols.append(TuringMachine::EMPTY_SYMBOL);

    // Доп. символы
    QStringList extraSymbols = m_extraSymbols.values();
    extraSymbols.sort();
    symbols.append(extraSymbols);

    m_programTable->setColumnCount(symbols.size());
    m_programTable->setHorizontalHeaderLabels(symbols);

    // Сохраняем текущие данные если есть
    QMap<QString, QMap<QString, QString>> oldData;
    for (int row = 0; row < m_programTable->rowCount(); ++row) {
        if (m_programTable->verticalHeaderItem(row)) {
            QString state = m_programTable->verticalHeaderItem(row)->text();
            for (int col = 0; col < m_programTable->columnCount(); ++col) {
                QTableWidgetItem *item = m_programTable->item(row, col);
                if (item && !item->text().isEmpty()) {
                    QString symbol = m_programTable->horizontalHeaderItem(col)->text();
                    oldData[state][symbol] = item->text();
                }
            }
        }
    }

    // Восстанавливаем строки
    m_programTable->setRowCount(m_statesList.size());
    for (int i = 0; i < m_statesList.size(); ++i) {
        m_programTable->setVerticalHeaderItem(i, new QTableWidgetItem(m_statesList[i]));
    }

    // Восстанавливаем данные
    for (int row = 0; row < m_programTable->rowCount(); ++row) {
        QString state = m_programTable->verticalHeaderItem(row)->text();
        for (int col = 0; col < m_programTable->columnCount(); ++col) {
            QString symbol = m_programTable->horizontalHeaderItem(col)->text();
            QTableWidgetItem *item = new QTableWidgetItem("");
            if (oldData.contains(state) && oldData[state].contains(symbol)) {
                item->setText(oldData[state][symbol]);
            }
            m_programTable->setItem(row, col, item);
        }
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
        tape.append(TuringMachine::EMPTY_SYMBOL);
    }

    // Collect program
    QMap<QString, QMap<QString, Transition>> program = collectProgram();

    if (program.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Программа пуста");
        return;
    }

    m_machine->setProgram(program);

    // Check for halt
    if (!m_machine->programHasHalt()) {
        QMessageBox::warning(this, "Ошибка",
                             "В программе нет команды остановки (состояние '!')\n"
                             "Допустимые форматы команд:\n"
                             "- R (сдвиг вправо)\n"
                             "- L (сдвиг влево)\n"
                             "- ! (остановка)\n"
                             "- R ! (сдвиг вправо и остановка)\n"
                             "- 1 R q1 (запись, сдвиг, состояние)");
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
        QTableWidgetItem *headerItem = m_programTable->verticalHeaderItem(row);
        if (!headerItem) continue;

        QString state = headerItem->text();
        QMap<QString, Transition> transitions;

        for (int col = 0; col < m_programTable->columnCount(); ++col) {
            QString symbol = m_programTable->horizontalHeaderItem(col)->text();
            QTableWidgetItem *item = m_programTable->item(row, col);

            if (item && !item->text().isEmpty()) {
                Transition t = TuringMachine::parseCommand(item->text().trimmed(), symbol, state);
                transitions[symbol] = t;
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
        tape.append(TuringMachine::EMPTY_SYMBOL);
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
    if (maxNum == -1) {
        return "q1";
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

    for (int col = 0; col < m_programTable->columnCount(); ++col) {
        if (!m_programTable->item(row, col)) {
            m_programTable->setItem(row, col, new QTableWidgetItem(""));
        }
    }
}

void MainWindow::removeState()
{
    if (m_programTable->rowCount() <= 1) {
        return;
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
    QTableWidgetItem *item = m_programTable->item(row, col);
    if (!item) return;

    QString text = item->text().trimmed();
    if (text.isEmpty()) {
        item->setBackground(Qt::white);
        return;
    }

    // Проверяем, что команда валидна
    Transition t = TuringMachine::parseCommand(text, "", "");

    // Проверяем направление
    if (t.direction != "L" && t.direction != "R" && t.direction != "N") {
        item->setBackground(Qt::yellow);
    } else {
        item->setBackground(Qt::white);
    }
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