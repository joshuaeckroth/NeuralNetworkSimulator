#include <cstdlib>
#include <ctime>
using namespace std;

#include <QtGui/QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    qsrand(time(NULL));
    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    return app.exec();
}
