#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_stepDelayMs(500)
{
    m_machine = new TuringMachine(this);
    m_tapeWidget = new TapeWidget(this);

    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // Alphabet controls
    QHBoxLayout *alphabetLayout = new QHBoxLayout();
    alphabetLayout->addWidget(new QLabel("Алфавит ленты (через запятую):"));
    m_tapeAlphabetEdit = new QLineEdit();
    alphabetLayout->addWidget(m_tapeAlphabetEdit);
    alphabetLayout->addWidget(new QLabel("Доп. символы:"));
    m_extraSymbolsEdit = new QLineEdit();
    alphabetLayout->addWidget(m_extraSymbolsEdit);
    m_setAlphabetsButton = new QPushButton("Задать алфавиты");
    alphabetLayout->addWidget(m_setAlphabetsButton);
    mainLayout->addLayout(alphabetLayout);

    // Tape widget
    mainLayout->addWidget(m_tapeWidget);

    // Program table
    m_programTable = new QTableWidget(0, 0);
    m_programTable->setEnabled(false);
    mainLayout->addWidget(m_programTable);

    QHBoxLayout *stateButtonsLayout = new QHBoxLayout();
    m_addStateButton = new QPushButton("+ Состояние");
    m_addStateButton->setEnabled(false);
    m_removeStateButton = new QPushButton("- Состояние");
    m_removeStateButton->setEnabled(false);
    stateButtonsLayout->addWidget(m_addStateButton);
    stateButtonsLayout->addWidget(m_removeStateButton);
    mainLayout->addLayout(stateButtonsLayout);

    // Input word
    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->addWidget(new QLabel("Входное слово:"));
    m_inputWordEdit = new QLineEdit();
    m_inputWordEdit->setEnabled(false);
    inputLayout->addWidget(m_inputWordEdit);
    m_setStringButton = new QPushButton("Задать строку");
    m_setStringButton->setEnabled(false);
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
    connect(m_setAlphabetsButton, &QPushButton::clicked, this, &MainWindow::setAlphabets);
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
}

MainWindow::~MainWindow() {}

void MainWindow::setAlphabets()
{
    QString tapeInput = m_tapeAlphabetEdit->text();
    QString extraInput = m_extraSymbolsEdit->text();

    QSet<QString> newTapeAlphabet;
    for (QString s : tapeInput.split(',', Qt::SkipEmptyParts)) {
        s = s.trimmed();
        if (!s.isEmpty()) newTapeAlphabet.insert(s);
    }
    newTapeAlphabet.insert("Λ");

    QSet<QString> newExtraSymbols;
    for (QString s : extraInput.split(',', Qt::SkipEmptyParts)) {
        s = s.trimmed();
        if (!s.isEmpty()) newExtraSymbols.insert(s);
    }

    // True: if only added symbols, keep table; else clear.
    bool onlyAdded = true;
    for (const QString& oldSym : m_tapeAlphabet) {
        if (!newTapeAlphabet.contains(oldSym)) {
            onlyAdded = false;
            break;
        }
    }
    for (const QString& oldSym : m_extraSymbols) {
        if (!newExtraSymbols.contains(oldSym)) {
            onlyAdded = false;
            break;
        }
    }

    m_tapeAlphabet = newTapeAlphabet;
    m_extraSymbols = newExtraSymbols;
    m_machine->setAlphabets(m_tapeAlphabet, m_extraSymbols);

    if (onlyAdded && m_programTable->rowCount() > 0) {
        // Just rebuild columns, keep data
        buildTable();
    } else {
        clearTable();
        buildTable();
    }

    enableInputs(true);
    m_setStringButton->setEnabled(true);
    m_addStateButton->setEnabled(true);
    m_removeStateButton->setEnabled(true);
    m_programTable->setEnabled(true);
    m_inputWordEdit->setEnabled(true);
}

void MainWindow::buildTable()
{
    m_programTable->blockSignals(true);
    m_programTable->clear();
    QStringList symbols = (m_tapeAlphabet + m_extraSymbols).values();
    symbols.sort();
    m_programTable->setColumnCount(symbols.size());
    m_programTable->setHorizontalHeaderLabels(symbols);

    // Keep existing rows if any
    if (m_programTable->rowCount() == 0) {
        m_programTable->setRowCount(1);
        m_programTable->setVerticalHeaderItem(0, new QTableWidgetItem("q0"));
    }
    m_programTable->blockSignals(false);
}

void MainWindow::clearTable()
{
    m_programTable->blockSignals(true);
    m_programTable->clearContents();
    m_programTable->setRowCount(0);
    m_programTable->blockSignals(false);
}

void MainWindow::enableInputs(bool enable)
{
    // Used when running/stopping
    m_tapeAlphabetEdit->setEnabled(enable);
    m_extraSymbolsEdit->setEnabled(enable);
    m_setAlphabetsButton->setEnabled(enable);
    m_inputWordEdit->setEnabled(enable);
    m_setStringButton->setEnabled(enable);
    m_programTable->setEnabled(enable);
    m_addStateButton->setEnabled(enable);
    m_removeStateButton->setEnabled(enable);
}

void MainWindow::setString()
{
    QString word = m_inputWordEdit->text();
    QVector<QString> tape;
    for (QChar ch : word) {
        QString sym(ch);
        if (!m_tapeAlphabet.contains(sym) && !m_extraSymbols.contains(sym)) {
            QMessageBox::warning(this, "Ошибка", "Символ не из алфавита: " + sym);
            return;
        }
        tape.append(sym);
    }
    if (tape.isEmpty()) tape.append("Λ");
    m_machine->setInitialTape(tape, 0);
    m_tapeWidget->setTape(m_machine->tape(), m_machine->headPosition());

    // Check if program has halt
    if (!m_machine->programHasHalt()) {
        QMessageBox::warning(this, "Ошибка", "В программе нет команды остановки (!HALT!)");
        return;
    }

    m_runButton->setEnabled(true);
    m_stepButton->setEnabled(true);
    m_resetButton->setEnabled(true);
    m_speedUpButton->setEnabled(true);
    m_slowDownButton->setEnabled(true);
}

void MainWindow::runMachine()
{
    if (m_machine->isHalted()) return;
    enableInputs(false);
    m_runButton->setEnabled(false);
    m_stopButton->setEnabled(true);
    m_stepButton->setEnabled(false);
    m_runTimer->start();
}

void MainWindow::stopMachine()
{
    m_runTimer->stop();
    enableInputs(true);
    m_runButton->setEnabled(true);
    m_stopButton->setEnabled(false);
    m_stepButton->setEnabled(true);
}

void MainWindow::stepMachine()
{
    if (m_machine->isHalted()) return;
    m_machine->step();
    // Animate head movement
    m_tapeWidget->setTape(m_machine->tape(), m_machine->headPosition());
    // In real implementation you'd animate between old and new positions.
    // Here we just update directly for brevity.
}

void MainWindow::resetMachine()
{
    stopMachine();
    m_machine->reset();
    m_tapeWidget->setTape(m_machine->tape(), m_machine->headPosition());
    enableInputs(true);
    m_runButton->setEnabled(true);
    m_stepButton->setEnabled(true);
    m_resetButton->setEnabled(true);
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

void MainWindow::addState()
{
    int row = m_programTable->rowCount();
    m_programTable->setRowCount(row + 1);
    m_programTable->setVerticalHeaderItem(row, new QTableWidgetItem("q" + QString::number(row)));
}

void MainWindow::removeState()
{
    if (m_programTable->rowCount() > 1) {
        m_programTable->setRowCount(m_programTable->rowCount() - 1);
    }
}

void MainWindow::onCellChanged(int row, int col)
{
    // Validate transition format: "symbol direction state"
    QString text = m_programTable->item(row, col)->text();
    // Basic validation could be added here.
    // For brevity, we'll just accept anything.
}

void MainWindow::onTapeAnimationFinished()
{
    // After animation, check if machine needs to continue running
    if (m_runTimer->isActive()) {
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
