#ifndef SETUPDIALOG_H
#define SETUPDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QSet>
#include <QString>

class SetupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SetupDialog(QWidget *parent = nullptr);

    QSet<QString> tapeAlphabet() const { return m_tapeAlphabet; }
    QSet<QString> extraSymbols() const { return m_extraSymbols; }

signals:
    void alphabetsSet();

private slots:
    void onSetClicked();

private:
    QLineEdit *m_tapeAlphabetEdit;
    QLineEdit *m_extraSymbolsEdit;
    QPushButton *m_setButton;

    QSet<QString> m_tapeAlphabet;
    QSet<QString> m_extraSymbols;

    QSet<QString> parseAlphabet(const QString& text);
};

#endif // SETUPDIALOG_H