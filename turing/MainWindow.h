#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QSpinBox>
#include "TuringMachine.h"
#include "TapeWidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void setAlphabets();
    void setString();
    void runMachine();
    void stopMachine();
    void stepMachine();
    void resetMachine();
    void speedUp();
    void slowDown();
    void addState();
    void removeState();
    void onCellChanged(int row, int col);
    void onTapeAnimationFinished();

private:
    TuringMachine *m_machine;
    TapeWidget *m_tapeWidget;

    QLineEdit *m_tapeAlphabetEdit;
    QLineEdit *m_extraSymbolsEdit;
    QPushButton *m_setAlphabetsButton;

    QTableWidget *m_programTable;
    QPushButton *m_addStateButton;
    QPushButton *m_removeStateButton;

    QLineEdit *m_inputWordEdit;
    QPushButton *m_setStringButton;

    QPushButton *m_runButton;
    QPushButton *m_stopButton;
    QPushButton *m_stepButton;
    QPushButton *m_resetButton;
    QPushButton *m_speedUpButton;
    QPushButton *m_slowDownButton;

    QTimer *m_runTimer;
    int m_stepDelayMs;

    QSet<QString> m_tapeAlphabet;
    QSet<QString> m_extraSymbols;

    void buildTable();
    void clearTable();
    void enableInputs(bool enable);
    bool validateTapeAlphabet(const QString& input);
    bool validateInputWord(const QString& word);
    void updateTableHighlight(const QString& state);
};

#endif // MAINWINDOW_H
