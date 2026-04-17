#include "SetupDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>

SetupDialog::SetupDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Настройка алфавитов");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Tape alphabet
    QHBoxLayout *tapeLayout = new QHBoxLayout();
    tapeLayout->addWidget(new QLabel("Алфавит строки (символы без разделителей):"));
    m_tapeAlphabetEdit = new QLineEdit();
    m_tapeAlphabetEdit->setPlaceholderText("Например: 01ab");
    tapeLayout->addWidget(m_tapeAlphabetEdit);
    mainLayout->addLayout(tapeLayout);

    // Extra symbols
    QHBoxLayout *extraLayout = new QHBoxLayout();
    extraLayout->addWidget(new QLabel("Алфавит доп. символов (символы без разделителей):"));
    m_extraSymbolsEdit = new QLineEdit();
    m_extraSymbolsEdit->setPlaceholderText("Например: XY");
    extraLayout->addWidget(m_extraSymbolsEdit);
    mainLayout->addLayout(extraLayout);

    // Set button
    m_setButton = new QPushButton("Задать алфавиты");
    mainLayout->addWidget(m_setButton);

    connect(m_setButton, &QPushButton::clicked, this, &SetupDialog::onSetClicked);
}

QSet<QString> SetupDialog::parseAlphabet(const QString& text)
{
    QSet<QString> result;
    for (QChar ch : text) {
        if (!ch.isSpace()) {
            result.insert(QString(ch));
        }
    }
    return result;
}

void SetupDialog::onSetClicked()
{
    m_tapeAlphabet = parseAlphabet(m_tapeAlphabetEdit->text());
    m_extraSymbols = parseAlphabet(m_extraSymbolsEdit->text());

    if (m_tapeAlphabet.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Алфавит строки не может быть пустым");
        return;
    }

    // Check for intersection
    QSet<QString> intersection = m_tapeAlphabet;
    intersection.intersect(m_extraSymbols);
    if (!intersection.isEmpty()) {
        QMessageBox::warning(this, "Ошибка",
                             "Алфавит строки и доп. символы не должны пересекаться");
        return;
    }

    emit alphabetsSet();
    accept();
}