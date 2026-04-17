#include <QApplication>
#include "SetupDialog.h"
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    SetupDialog setupDialog;
    if (setupDialog.exec() != QDialog::Accepted) {
        return 0;
    }

    MainWindow w(setupDialog.tapeAlphabet(), setupDialog.extraSymbols());
    w.show();

    return a.exec();
}